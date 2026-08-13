#include "SCCP.h"
#include "Worklist.h"

#include <cctype>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace
{

bool hasSideEffect(
    InstType type)
{
    return type == InstType::Call ||
           type == InstType::Branch ||
           type == InstType::Return ||
           type == InstType::FieldStore ||
           type == InstType::Alloc;
}

}

void SCCP::run(FunctionIR& F)
{
    cells_.clear();
    stats_.reset();
    stats_.add("instructions", F.instructions.size());

    std::unordered_map<std::string, std::vector<int>> users;

    for(std::size_t i = 0; i < F.instructions.size(); ++i)
    {
        for(const auto& use : F.instructions[i].uses)
        {
            users[use].push_back(static_cast<int>(i));
        }
    }

    Worklist worklist;

    for(std::size_t i = 0; i < F.instructions.size(); ++i)
    {
        worklist.push(static_cast<int>(i));
    }

    while(!worklist.empty())
    {
        const int index = worklist.pop();

        if(index < 0 ||
           index >= static_cast<int>(F.instructions.size()))
        {
            continue;
        }

        const Instruction& inst = F.instructions[static_cast<std::size_t>(index)];

        if(inst.result.empty())
        {
            continue;
        }

        const LatticeValue next = interpret(inst);

        if(update(inst.result, next))
        {
            const auto found = users.find(inst.result);

            if(found != users.end())
            {
                for(int user : found->second)
                {
                    worklist.push(user);
                }
            }
        }
    }

    for(auto& inst : F.instructions)
    {
        if(hasSideEffect(inst.type) || inst.result.empty())
        {
            continue;
        }

        bool used = false;

        for(const auto& other : F.instructions)
        {
            if(other.uses.count(inst.result) != 0)
            {
                used = true;
                break;
            }
        }

        if(!used && isPure(inst.type))
        {
            inst.dead = true;
            stats_.add("dead");
        }

        const LatticeValue cell = get(inst.result);

        if(cell.state == Lattice::Constant)
        {
            stats_.add("constants");
        }
        else if(cell.state == Lattice::Overdefined)
        {
            stats_.add("overdefined");
        }
    }
}

LatticeValue SCCP::get(
    const std::string& name) const
{
    return lookup(name);
}

int SCCP::constantCount() const
{
    return static_cast<int>(stats_.get("constants"));
}

const Statistics& SCCP::stats() const
{
    return stats_;
}

void SCCP::print() const
{
    std::cout
        << "SCCP lattice:\n";

    if(cells_.empty())
    {
        std::cout
            << "  (empty)\n";
        return;
    }

    for(const auto& entry : cells_)
    {
        std::cout
            << "  "
            << entry.first
            << " : "
            << latticeName(entry.second.state);

        if(entry.second.state == Lattice::Constant)
        {
            std::cout
                << " = "
                << entry.second.value;
        }

        std::cout
            << "\n";
    }

    stats_.print("SCCP statistics");
}

LatticeValue SCCP::meet(
    const LatticeValue& left,
    const LatticeValue& right) const
{
    if(left.state == Lattice::Unknown)
    {
        return right;
    }

    if(right.state == Lattice::Unknown)
    {
        return left;
    }

    if(left.state == Lattice::Overdefined ||
       right.state == Lattice::Overdefined)
    {
        LatticeValue top;
        top.state = Lattice::Overdefined;
        return top;
    }

    if(left.value == right.value)
    {
        return left;
    }

    LatticeValue top;
    top.state = Lattice::Overdefined;
    return top;
}

LatticeValue SCCP::lookup(
    const std::string& name) const
{
    int literal = 0;

    if(parseConstant(name, literal))
    {
        LatticeValue cell;
        cell.state = Lattice::Constant;
        cell.value = literal;
        return cell;
    }

    const auto found = cells_.find(name);

    if(found == cells_.end())
    {
        return LatticeValue();
    }

    return found->second;
}

LatticeValue SCCP::interpret(
    const Instruction& inst) const
{
    LatticeValue result;

    switch(inst.type)
    {
    case InstType::Assign:
    case InstType::Copy:
    {
        for(const auto& use : inst.uses)
        {
            result = meet(result, lookup(use));
        }

        return result;
    }

    case InstType::Add:
    case InstType::Sub:
    case InstType::Mul:
    case InstType::Div:
    {
        std::vector<LatticeValue> operands;

        for(const auto& use : inst.uses)
        {
            operands.push_back(lookup(use));
        }

        if(operands.empty())
        {
            return result;
        }

        bool allConstant = true;
        bool anyOverdefined = false;

        for(const auto& operand : operands)
        {
            if(operand.state == Lattice::Overdefined)
            {
                anyOverdefined = true;
            }

            if(operand.state != Lattice::Constant)
            {
                allConstant = false;
            }
        }

        if(anyOverdefined)
        {
            result.state = Lattice::Overdefined;
            return result;
        }

        if(!allConstant)
        {
            return result;
        }

        int value = operands.front().value;

        for(std::size_t i = 1; i < operands.size(); ++i)
        {
            const int rhs = operands[i].value;

            if(inst.type == InstType::Add)
            {
                value += rhs;
            }
            else if(inst.type == InstType::Sub)
            {
                value -= rhs;
            }
            else if(inst.type == InstType::Mul)
            {
                value *= rhs;
            }
            else if(rhs == 0)
            {
                result.state = Lattice::Overdefined;
                return result;
            }
            else
            {
                value /= rhs;
            }
        }

        result.state = Lattice::Constant;
        result.value = value;
        return result;
    }

    case InstType::Call:
    case InstType::Branch:
    case InstType::Return:
    case InstType::Alloc:
    case InstType::FieldStore:
    case InstType::FieldLoad:
    default:
    {
        result.state = Lattice::Overdefined;
        return result;
    }
    }
}

bool SCCP::update(
    const std::string& name,
    const LatticeValue& value)
{
    LatticeValue& cell = cells_[name];
    const LatticeValue merged = meet(cell, value);

    if(cell.state == merged.state &&
       (cell.state != Lattice::Constant || cell.value == merged.value))
    {
        return false;
    }

    cell = merged;
    return true;
}

bool SCCP::parseConstant(
    const std::string& text,
    int& value)
{
    if(text.empty())
    {
        return false;
    }

    std::size_t index = 0;

    if(text[0] == '-' || text[0] == '+')
    {
        if(text.size() == 1)
        {
            return false;
        }

        index = 1;
    }

    for(std::size_t i = index; i < text.size(); ++i)
    {
        if(!std::isdigit(static_cast<unsigned char>(text[i])))
        {
            return false;
        }
    }

    try
    {
        value = std::stoi(text);
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool SCCP::isPure(
    InstType type)
{
    return type == InstType::Assign ||
           type == InstType::Add ||
           type == InstType::Sub ||
           type == InstType::Mul ||
           type == InstType::Div ||
           type == InstType::Copy;
}

const char* SCCP::latticeName(
    Lattice state)
{
    switch(state)
    {
    case Lattice::Unknown:     return "Unknown";
    case Lattice::Constant:    return "Constant";
    case Lattice::Overdefined: return "Overdefined";
    }

    return "Unknown";
}
