#pragma once

#include "IR.h"
#include "Statistics.h"

#include <string>
#include <unordered_map>
#include <vector>

enum class Lattice
{
    Unknown,
    Constant,
    Overdefined
};

struct LatticeValue
{
    Lattice state = Lattice::Unknown;

    int value = 0;
};

class SCCP
{
public:

    void run(FunctionIR& F);

    LatticeValue get(
        const std::string& name) const;

    int constantCount() const;

    const Statistics& stats() const;

    void print() const;

private:

    LatticeValue meet(
        const LatticeValue& left,
        const LatticeValue& right) const;

    LatticeValue lookup(
        const std::string& name) const;

    LatticeValue interpret(
        const Instruction& inst) const;

    bool update(
        const std::string& name,
        const LatticeValue& value);

    static bool parseConstant(
        const std::string& text,
        int& value);

    static bool isPure(
        InstType type);

    static const char* latticeName(
        Lattice state);

private:

    std::unordered_map<std::string, LatticeValue> cells_;

    Statistics stats_;
};
