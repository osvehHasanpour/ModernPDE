#pragma once

#include <string>
#include <vector>
#include <set>

enum class InstType
{
    Assign,
    Add,
    Sub,
    Mul,
    Div,
    Call,
    Branch,
    Return,
    Alloc,
    Copy,
    FieldStore,
    FieldLoad
};

struct Instruction
{
    int id;

    InstType type;

    std::string result;

    std::set<std::string> uses;

    bool dead = false;
    bool partialDead = false;
};

class FunctionIR
{
public:

    std::string name;

    std::vector<Instruction> instructions;

    void print() const;

    Instruction* findById(int id);

    const Instruction* findById(int id) const;
};