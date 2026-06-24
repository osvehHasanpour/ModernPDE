#include "CFGPDE.h"

void CFGPDE::registerVariable(
    const std::string& name,
    int totalPaths,
    int usedPaths)
{
    pde.defineVariable(name);

    for(int i=0;i<totalPaths;i++)
    {
        pde.addExecutionPath(name);
    }

    for(int i=0;i<usedPaths;i++)
    {
        pde.useVariable(name);
    }
}

void CFGPDE::analyze()
{
    pde.printResults();
}