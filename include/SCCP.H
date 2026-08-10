#pragma once

#include "IR.h"

enum class Lattice
{
    Unknown,
    Constant,
    Overdefined
};

class SCCP
{
public:

    void run(FunctionIR& F);
};