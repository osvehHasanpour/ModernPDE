#include "FieldSensitiveAnalysis.h"

#include <iostream>
#include <queue>
#include <utility>

namespace
{

const char* fieldStateName(FieldState state)
{
    switch(state)
    {
    case FieldState::Uninitialized:
        return "Uninitialized";

    case FieldState::Written:
        return "Written";

    case FieldState::Read:
        return "Read";

    case FieldState::WrittenAndRead:
        return "WrittenAndRead";

    case FieldState::Top:
        return "Top";
    }

    return "Unknown";
}

const char* escapeStateName(EscapeState state)
{
    switch(state)
    {
    case EscapeState::Local:
        return "Local";

    case EscapeState::Escaped:
        return "Escaped";
    }

    return "Unknown";
}

const char* instTypeName(InstType type)
{
    switch(type)
    {
    case InstType::Alloc:
        return "Alloc";

    case InstType::Copy:
        return "Copy";

    case InstType::FieldStore:
        return "FieldStore";

    case InstType::FieldLoad:
        return "FieldLoad";

    case InstType::Call:
        return "Call";

  default:
        return "Other";
    }
}

}

const std::vector<AbstractObject>&
FieldSensitiveAnalysis::objects() const
{
    return objects_;
}

const std::unordered_map<
    std::string,
    std::set<int>
>& FieldSensitiveAnalysis::pointsTo() const
{
    return pts_;
}

const std::map<
    FieldKey,
    FieldState
>& FieldSensitiveAnalysis::fieldStates() const
{
    return fieldStates_;
}

const std::vector<FieldAccessLog>&
FieldSensitiveAnalysis::fieldReads() const
{
    return fieldReads_;
}

void FieldSensitiveAnalysis::reset(FunctionIR& F)
{
    objects_.clear();
    pts_.clear();
    fieldStates_.clear();
    fieldReads_.clear();
    copyEdges_.clear();

    AbstractObject summary;

    summary.id = 0;
    summary.allocSite = "@summary";
    summary.escape = EscapeState::Escaped;

    objects_.push_back(summary);
    summaryObjectId_ = 0;

    for(auto& inst : F.instructions)
    {
        inst.dead = false;
        inst.partialDead = false;
    }
}

void FieldSensitiveAnalysis::collectAllocations(
    FunctionIR& F)
{
    int nextId =
        static_cast<int>(objects_.size());

    for(const auto& inst : F.instructions)
    {
        if(inst.type != InstType::Alloc)
        {
            continue;
        }

        if(inst.uses.empty())
        {
            continue;
        }

        AbstractObject object;

        object.id = nextId++;
        object.allocSite = *inst.uses.begin();
        object.escape = EscapeState::Local;

        objects_.push_back(object);

        pts_[inst.result].insert(object.id);
    }
}

bool FieldSensitiveAnalysis::classifyInstruction(
    const Instruction& inst,
    InstType& kind,
    std::string& base,
    std::string& field,
    std::string& value,
    std::string& allocSite) const
{
    base.clear();
    field.clear();
    value.clear();
    allocSite.clear();

    kind = inst.type;

    if(inst.type == InstType::Alloc)
    {
        if(!inst.uses.empty())
        {
            allocSite = *inst.uses.begin();
        }

        return true;
    }

    if(inst.type == InstType::Copy)
    {
        if(!inst.uses.empty())
        {
            value = *inst.uses.begin();
        }

        return true;
    }

    if(inst.type == InstType::FieldStore)
    {
        const auto dot =
            inst.result.find('.');

        if(dot == std::string::npos)
        {
            return false;
        }

        base = inst.result.substr(0, dot);
        field = inst.result.substr(dot + 1);

        if(!inst.uses.empty())
        {
            value = *inst.uses.begin();
        }

        return !base.empty() && !field.empty();
    }

    if(inst.type == InstType::FieldLoad)
    {
        if(inst.uses.empty())
        {
            return false;
        }

        const std::string payload =
            *inst.uses.begin();

        const auto dot =
            payload.find('.');

        if(dot == std::string::npos)
        {
            return false;
        }

        base = payload.substr(0, dot);
        field = payload.substr(dot + 1);

        return !base.empty() && !field.empty();
    }

    if(inst.type != InstType::Assign)
    {
        return false;
    }

    if(inst.result.rfind("@alloc:", 0) == 0)
    {
        kind = InstType::Alloc;
        allocSite = inst.result.substr(7);
        return true;
    }

    if(inst.result.rfind("@copy:", 0) == 0)
    {
        kind = InstType::Copy;
        value = inst.result.substr(6);
        return true;
    }

    if(inst.result.rfind("@fldstore:", 0) == 0)
    {
        kind = InstType::FieldStore;

        const std::string payload =
            inst.result.substr(10);

        const auto dot =
            payload.find('.');

        if(dot == std::string::npos)
        {
            return false;
        }

        base = payload.substr(0, dot);
        field = payload.substr(dot + 1);

        if(!inst.uses.empty())
        {
            value = *inst.uses.begin();
        }

        return true;
    }

    if(inst.result.rfind("@fldload:", 0) == 0)
    {
        kind = InstType::FieldLoad;

        const std::string payload =
            inst.result.substr(9);

        const auto dot =
            payload.find('.');

        if(dot == std::string::npos)
        {
            return false;
        }

        base = payload.substr(0, dot);
        field = payload.substr(dot + 1);

        return true;
    }

    return false;
}

void FieldSensitiveAnalysis::propagatePointsTo(
    const std::string& target,
    const std::string& source)
{
    const auto& sourceSet =
        pts_[source];

    pts_[target].insert(
        sourceSet.begin(),
        sourceSet.end());
}

void FieldSensitiveAnalysis::runPointsTo(
    FunctionIR& F)
{
    copyEdges_.clear();

    for(const auto& inst : F.instructions)
    {
        InstType kind = inst.type;
        std::string base;
        std::string field;
        std::string value;
        std::string allocSite;

        if(!classifyInstruction(
            inst,
            kind,
            base,
            field,
            value,
            allocSite))
        {
            continue;
        }

        if(kind == InstType::Alloc)
        {
            if(pts_[inst.result].empty())
            {
                AbstractObject object;

                object.id =
                    static_cast<int>(
                        objects_.size());

                object.allocSite = allocSite;
                object.escape = EscapeState::Local;

                objects_.push_back(object);

                pts_[inst.result].insert(
                    object.id);
            }

            continue;
        }

        if(kind == InstType::Copy)
        {
            copyEdges_[inst.result] = value;
            propagatePointsTo(
                inst.result,
                value);
        }
    }

    std::queue<std::string> worklist;

    for(const auto& edge : copyEdges_)
    {
        worklist.push(edge.first);
    }

    while(!worklist.empty())
    {
        const std::string target =
            worklist.front();

        worklist.pop();

        const auto found =
            copyEdges_.find(target);

        if(found == copyEdges_.end())
        {
            continue;
        }

        const std::size_t before =
            pts_[target].size();

        const auto& sourceSet =
            pts_[found->second];

        pts_[target].insert(
            sourceSet.begin(),
            sourceSet.end());

        if(pts_[target].size() != before)
        {
            for(const auto& edge : copyEdges_)
            {
                if(edge.second == target)
                {
                    worklist.push(edge.first);
                }
            }
        }
    }
}

bool FieldSensitiveAnalysis::isEscapeCall(
    const Instruction& inst) const
{
    return
        inst.type == InstType::Call &&
        inst.result == "@escape";
}

bool FieldSensitiveAnalysis::isHeapPointerVar(
    const std::string& var) const
{
    const auto found =
        pts_.find(var);

    if(found == pts_.end())
    {
        return false;
    }

    for(int objectId : found->second)
    {
        if(objectId != summaryObjectId_)
        {
            return true;
        }
    }

    return false;
}

void FieldSensitiveAnalysis::runEscapeAnalysis(
    FunctionIR& F)
{
    for(const auto& inst : F.instructions)
    {
        if(isEscapeCall(inst))
        {
            for(const auto& var : inst.uses)
            {
                for(int objectId :
                    pointsToVar(var))
                {
                    if(objectId >= 0 &&
                       objectId <
                       static_cast<int>(
                           objects_.size()))
                    {
                        objects_[
                            static_cast<
                                std::size_t
                            >(objectId)
                        ].escape =
                            EscapeState::Escaped;
                    }
                }
            }

            continue;
        }

        if(inst.type != InstType::Call)
        {
            continue;
        }

        if(inst.result == "@escape")
        {
            continue;
        }

        for(const auto& var : inst.uses)
        {
            if(!isHeapPointerVar(var))
            {
                continue;
            }

            for(int objectId :
                pointsToVar(var))
            {
                if(objectId >= 0 &&
                   objectId <
                   static_cast<int>(
                       objects_.size()))
                {
                    objects_[
                        static_cast<
                            std::size_t
                        >(objectId)
                    ].escape =
                        EscapeState::Escaped;
                }
            }
        }
    }
}

FieldState FieldSensitiveAnalysis::joinFieldState(
    FieldState current,
    FieldState next) const
{
    if(current == FieldState::Top ||
       next == FieldState::Top)
    {
        return FieldState::Top;
    }

    if(current == next)
    {
        return current;
    }

    if(
        (current == FieldState::Written &&
         next == FieldState::Read) ||
        (current == FieldState::Read &&
         next == FieldState::Written))
    {
        return FieldState::WrittenAndRead;
    }

    if(
        current == FieldState::WrittenAndRead ||
        next == FieldState::WrittenAndRead)
    {
        return FieldState::WrittenAndRead;
    }

    if(
        current == FieldState::Uninitialized)
    {
        return next;
    }

    if(
        next == FieldState::Uninitialized)
    {
        return current;
    }

    return FieldState::Top;
}

void FieldSensitiveAnalysis::markFieldState(
    int objectId,
    const std::string& field,
    FieldState next,
    bool strongUpdate)
{
    FieldKey key;

    key.objectId = objectId;
    key.field = field;

    if(!strongUpdate)
    {
        fieldStates_[key] = FieldState::Top;
        return;
    }

    fieldStates_[key] =
        joinFieldState(
            fieldStates_[key],
            next);
}

std::set<int> FieldSensitiveAnalysis::pointsToVar(
    const std::string& var) const
{
    const auto found =
        pts_.find(var);

    if(found == pts_.end())
    {
        return {summaryObjectId_};
    }

    if(found->second.empty())
    {
        return {summaryObjectId_};
    }

    return found->second;
}

void FieldSensitiveAnalysis::runFieldStateAnalysis(
    FunctionIR& F)
{
    for(const auto& inst : F.instructions)
    {
        InstType kind = inst.type;
        std::string base;
        std::string field;
        std::string value;
        std::string allocSite;

        if(!classifyInstruction(
            inst,
            kind,
            base,
            field,
            value,
            allocSite))
        {
            continue;
        }

        if(kind == InstType::FieldStore)
        {
            const std::set<int> targets =
                pointsToVar(base);

            const bool strong =
                targets.size() == 1 &&
                objects_[
                    static_cast<std::size_t>(
                        *targets.begin())
                ].escape ==
                    EscapeState::Local;

            for(int objectId : targets)
            {
                markFieldState(
                    objectId,
                    field,
                    FieldState::Written,
                    strong);
            }

            continue;
        }

        if(kind == InstType::FieldLoad)
        {
            const std::set<int> targets =
                pointsToVar(base);

            const bool strong =
                targets.size() == 1 &&
                objects_[
                    static_cast<std::size_t>(
                        *targets.begin())
                ].escape ==
                    EscapeState::Local;

            for(int objectId : targets)
            {
                markFieldState(
                    objectId,
                    field,
                    FieldState::Read,
                    strong);
            }
        }
    }
}

void FieldSensitiveAnalysis::runFieldReadLogging(
    FunctionIR& F)
{
    fieldReads_.clear();

    for(const auto& inst : F.instructions)
    {
        InstType kind = inst.type;
        std::string base;
        std::string field;
        std::string value;
        std::string allocSite;

        if(!classifyInstruction(
            inst,
            kind,
            base,
            field,
            value,
            allocSite))
        {
            continue;
        }

        if(kind != InstType::FieldLoad)
        {
            continue;
        }

        FieldAccessLog log;

        log.insnId = inst.id;
        log.baseVar = base;
        log.field = field;
        log.resolvedObjects =
            pointsToVar(base);

        fieldReads_.push_back(log);

        std::cout
            << "[FieldRead] insn "
            << inst.id
            << " : load "
            << field
            << " from "
            << base
            << " -> {";

        bool first = true;

        for(int objectId :
            log.resolvedObjects)
        {
            if(!first)
            {
                std::cout << ", ";
            }

            first = false;

            std::cout
                << objects_[
                    static_cast<std::size_t>(
                        objectId)
                ].allocSite;
        }

        std::cout
            << "}\n";
    }
}

void FieldSensitiveAnalysis::runFieldSensitiveDeadness(
    FunctionIR& F)
{
    std::set<std::string> liveVars;
    std::set<FieldKey> liveFields;

    for(auto it =
        F.instructions.rbegin();
        it != F.instructions.rend();
        ++it)
    {
        auto& inst = *it;

        InstType kind = inst.type;
        std::string base;
        std::string field;
        std::string value;
        std::string allocSite;

        if(!classifyInstruction(
            inst,
            kind,
            base,
            field,
            value,
            allocSite))
        {
            continue;
        }

        if(kind == InstType::FieldLoad)
        {
            const std::set<int> targets =
                pointsToVar(base);

            for(int objectId : targets)
            {
                FieldKey key;

                key.objectId = objectId;
                key.field = field;

                liveFields.insert(key);
            }

            if(liveVars.count(inst.result) > 0)
            {
                liveVars.erase(inst.result);
                liveVars.insert(base);
            }

            continue;
        }

        if(kind == InstType::FieldStore)
        {
            const std::set<int> targets =
                pointsToVar(base);

            bool fieldNeeded = false;

            for(int objectId : targets)
            {
                FieldKey key;

                key.objectId = objectId;
                key.field = field;

                if(liveFields.count(key) > 0)
                {
                    fieldNeeded = true;
                    break;
                }
            }

            const bool valueNeeded =
                liveVars.count(value) > 0;

            if(!fieldNeeded && !valueNeeded)
            {
                bool allLocalSingleton = true;
                bool anyEscaped = false;

                if(targets.size() != 1)
                {
                    allLocalSingleton = false;
                }

                for(int objectId : targets)
                {
                    if(
                        objects_[
                            static_cast<
                                std::size_t
                            >(objectId)
                        ].escape ==
                            EscapeState::Escaped)
                    {
                        anyEscaped = true;
                    }
                }

                if(
                    allLocalSingleton &&
                    !anyEscaped)
                {
                    inst.dead = true;
                }
                else
                {
                    inst.partialDead = true;
                }
            }

            for(int objectId : targets)
            {
                FieldKey key;

                key.objectId = objectId;
                key.field = field;

                liveFields.erase(key);
            }

            if(valueNeeded)
            {
                liveVars.insert(value);
            }

            liveVars.insert(base);

            continue;
        }

        if(kind == InstType::Copy)
        {
            if(liveVars.count(inst.result) > 0)
            {
                liveVars.erase(inst.result);
                liveVars.insert(value);
            }

            continue;
        }

        if(kind == InstType::Call)
        {
            for(const auto& var : inst.uses)
            {
                liveVars.insert(var);
            }
        }
    }
}

void FieldSensitiveAnalysis::printResults(
    std::ostream& out) const
{
    out
        << "\n====================================\n"
        << "FIELD-SENSITIVE ANALYSIS RESULTS\n"
        << "====================================\n";

    out
        << "\nAbstract Objects:\n";

    for(const auto& object : objects_)
    {
        out
            << "  id="
            << object.id
            << " site="
            << object.allocSite
            << " escape="
            << escapeStateName(
                object.escape)
            << "\n";
    }

    out
        << "\nPoints-To Sets:\n";

    for(const auto& entry : pts_)
    {
        out
            << "  "
            << entry.first
            << " -> {";

        bool first = true;

        for(int objectId : entry.second)
        {
            if(!first)
            {
                out << ", ";
            }

            first = false;

            out
                << objects_[
                    static_cast<std::size_t>(
                        objectId)
                ].allocSite;
        }

        out
            << "}\n";
    }

    out
        << "\nField States:\n";

    for(const auto& entry : fieldStates_)
    {
        out
            << "  ("
            << objects_[
                static_cast<std::size_t>(
                    entry.first.objectId)
            ].allocSite
            << ", "
            << entry.first.field
            << ") = "
            << fieldStateName(
                entry.second)
            << "\n";
    }

    out
        << "\nField Read Log:\n";

    for(const auto& log : fieldReads_)
    {
        out
            << "  insn "
            << log.insnId
            << " base="
            << log.baseVar
            << " field="
            << log.field
            << " objects={";

        bool first = true;

        for(int objectId :
            log.resolvedObjects)
        {
            if(!first)
            {
                out << ", ";
            }

            first = false;

            out
                << objects_[
                    static_cast<std::size_t>(
                        objectId)
                ].allocSite;
        }

        out
            << "}\n";
    }
}

void FieldSensitiveAnalysis::run(FunctionIR& F)
{
    reset(F);

    collectAllocations(F);

    runPointsTo(F);

    runEscapeAnalysis(F);

    runFieldStateAnalysis(F);

    runFieldReadLogging(F);

    runFieldSensitiveDeadness(F);

    std::cout
        << "\nInstruction Flags:\n";

    for(const auto& inst : F.instructions)
    {
        InstType kind = inst.type;
        std::string base;
        std::string field;
        std::string value;
        std::string allocSite;

        if(!classifyInstruction(
            inst,
            kind,
            base,
            field,
            value,
            allocSite))
        {
            continue;
        }

        std::cout
            << "  insn "
            << inst.id
            << " "
            << instTypeName(kind)
            << " dead="
            << inst.dead
            << " partialDead="
            << inst.partialDead
            << "\n";
    }

    printResults(std::cout);
}
