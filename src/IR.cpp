#include "IR.h"

#include <iostream>

void FunctionIR::print() const
{
    std::cout
        << "FunctionIR "
        << name
        << " ("
        << instructions.size()
        << " instructions)\n";

    for(const auto& inst : instructions)
    {
        std::cout
            << "  ["
            << inst.id
            << "] ";

        switch(inst.type)
        {
        case InstType::Assign:     std::cout << "assign"; break;
        case InstType::Add:        std::cout << "add"; break;
        case InstType::Sub:        std::cout << "sub"; break;
        case InstType::Mul:        std::cout << "mul"; break;
        case InstType::Div:        std::cout << "div"; break;
        case InstType::Call:       std::cout << "call"; break;
        case InstType::Branch:     std::cout << "branch"; break;
        case InstType::Return:     std::cout << "return"; break;
        case InstType::Alloc:      std::cout << "alloc"; break;
        case InstType::Copy:       std::cout << "copy"; break;
        case InstType::FieldStore: std::cout << "fieldstore"; break;
        case InstType::FieldLoad:  std::cout << "fieldload"; break;
        }

        if(!inst.result.empty())
        {
            std::cout
                << " -> "
                << inst.result;
        }

        if(!inst.uses.empty())
        {
            std::cout << " uses {";
            bool first = true;
            for(const auto& use : inst.uses)
            {
                if(!first)
                {
                    std::cout << ", ";
                }
                first = false;
                std::cout << use;
            }
            std::cout << "}";
        }

        if(inst.dead)
        {
            std::cout << " [dead]";
        }
        else if(inst.partialDead)
        {
            std::cout << " [partialDead]";
        }

        std::cout << "\n";
    }
}

Instruction* FunctionIR::findById(int id)
{
    for(auto& inst : instructions)
    {
        if(inst.id == id)
        {
            return &inst;
        }
    }

    return nullptr;
}

const Instruction* FunctionIR::findById(int id) const
{
    for(const auto& inst : instructions)
    {
        if(inst.id == id)
        {
            return &inst;
        }
    }

    return nullptr;
}
