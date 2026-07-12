#pragma once

// IR encoding for heap / field instructions (see also InstType in IR.h):
//
//   Alloc       result = pointer var        uses = { "<Class>#<siteId>" }
//   Copy        result = lhs                uses = { rhs }
//   FieldStore  result = "<base>.<field>"     uses = { value }
//   FieldLoad   result = dst                  uses = { "<base>.<field>" }
//   Call        result = callee             uses = argument vars
//               Escape: result = "@escape"  uses = { pointer to escape }
//
// Each allocation-site tag (e.g. "Point#0" vs "Point#1") maps to a distinct
// abstract object even when the class name is the same.
//
// Legacy Assign fallback (prefix in result):
//   @alloc:<site>   @copy:<rhs>   @fldstore:<base>.<field>   @fldload:<base>.<field>

#include "IR.h"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

enum class FieldState
{
    Uninitialized,
    Written,
    Read,
    WrittenAndRead,
    Top
};

enum class EscapeState
{
    Local,
    Escaped
};

struct AbstractObject
{
    int id;

    std::string allocSite;

    EscapeState escape = EscapeState::Local;
};

struct FieldKey
{
    int objectId;

    std::string field;

    bool operator<(const FieldKey& other) const
    {
        if(objectId != other.objectId)
        {
            return objectId < other.objectId;
        }

        return field < other.field;
    }
};

struct FieldAccessLog
{
    int insnId;

    std::string baseVar;

    std::string field;

    std::set<int> resolvedObjects;
};

class FieldSensitiveAnalysis
{
public:

    void run(FunctionIR& F);

    const std::vector<AbstractObject>& objects() const;

    const std::unordered_map<
        std::string,
        std::set<int>
    >& pointsTo() const;

    const std::map<
        FieldKey,
        FieldState
    >& fieldStates() const;

    const std::vector<FieldAccessLog>& fieldReads() const;

    void printResults(std::ostream& out) const;

private:

    void reset(FunctionIR& F);

    void collectAllocations(FunctionIR& F);

    void runPointsTo(FunctionIR& F);

    void runEscapeAnalysis(FunctionIR& F);

    void runFieldStateAnalysis(FunctionIR& F);

    void runFieldReadLogging(FunctionIR& F);

    void runFieldSensitiveDeadness(FunctionIR& F);

    bool classifyInstruction(
        const Instruction& inst,
        InstType& kind,
        std::string& base,
        std::string& field,
        std::string& value,
        std::string& allocSite) const;

    bool isEscapeCall(const Instruction& inst) const;

    bool isHeapPointerVar(
        const std::string& var) const;

    FieldState joinFieldState(
        FieldState current,
        FieldState next) const;

    void markFieldState(
        int objectId,
        const std::string& field,
        FieldState next,
        bool strongUpdate);

    std::set<int> pointsToVar(
        const std::string& var) const;

    void propagatePointsTo(
        const std::string& target,
        const std::string& source);

private:

    std::vector<AbstractObject> objects_;

    int summaryObjectId_ = 0;

    std::unordered_map<
        std::string,
        std::set<int>
    > pts_;

    std::map<
        FieldKey,
        FieldState
    > fieldStates_;

    std::vector<FieldAccessLog> fieldReads_;

    std::unordered_map<
        std::string,
        std::string
    > copyEdges_;
};
