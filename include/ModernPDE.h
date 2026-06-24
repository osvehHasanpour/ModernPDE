#pragma once

#include "IR.h"

class ModernPDE
{
public:

    void run(FunctionIR& F);

private:

    void analyzeVirtualCalls(
        FunctionIR& F);

    void detectPartialDead(
        FunctionIR& F);

    void prioritize();

    void eliminate(
        FunctionIR& F);
};