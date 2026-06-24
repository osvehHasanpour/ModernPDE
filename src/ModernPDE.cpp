#include "../include/ModernPDE.h"


#include <iostream>

void ModernPDE::analyzeVirtualCalls(
    FunctionIR& F)
{
    std::cout
        << "Analyzing virtual calls\n";
}

void ModernPDE::detectPartialDead(
    FunctionIR& F)
{
    std::cout
        << "Detecting partial dead code\n";
}

void ModernPDE::prioritize()
{
    std::cout
        << "Prioritizing candidates\n";
}

void ModernPDE::eliminate(
    FunctionIR& F)
{
    std::cout
        << "Eliminating code\n";
}

void ModernPDE::run(
    FunctionIR& F)
{
    analyzeVirtualCalls(F);

    detectPartialDead(F);

    prioritize();

    eliminate(F);
}
