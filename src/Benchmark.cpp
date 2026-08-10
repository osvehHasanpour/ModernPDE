#include "Benchmark.h"
#include "PDE.h"

#include <iostream>
#include <chrono>

void Benchmark::run(
    int variableCount)
{
    PDE pde;

    auto start =
        std::chrono::high_resolution_clock::now();

    for(int i=0;i<variableCount;i++)
    {
        std::string name =
            "var_" +
            std::to_string(i);

        pde.defineVariable(name);

        for(int p=0;p<10;p++)
        {
            pde.addExecutionPath(name);
        }

        for(int u=0;u<5;u++)
        {
            pde.useVariable(name);
        }
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast
        <
            std::chrono::milliseconds
        >
        (end - start);

    std::cout
    << "Variables: "
    << variableCount
    << "  Time: "
    << duration.count()
    << " ms\n";
}