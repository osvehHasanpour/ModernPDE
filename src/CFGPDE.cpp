#include "CFGPDE.h"

#include <unordered_map>
#include <unordered_set>

void CFGPDE::registerVariable(
    const std::string& name,
    int totalPaths,
    int usedPaths)
{
    pde.defineVariable(name);

    for(int i = 0; i < totalPaths; i++)
    {
        pde.addExecutionPath(name);
    }

    for(int i = 0; i < usedPaths; i++)
    {
        pde.useVariable(name);
    }
}

void CFGPDE::analyzeFromCFG(
    const CFG& cfg,
    const PathEnumeration& paths,
    const DUChainAnalysis& du,
    int entryBlock)
{
    (void)cfg;
    (void)entryBlock;

    pde = PDE();

    const auto& allPaths =
        paths.getPaths();

    std::unordered_map<
        std::string,
        std::unordered_set<int>> useBlocks;

    std::unordered_map<
        std::string,
        std::unordered_set<int>> defBlocks;

    std::unordered_set<std::string> variables;

    for(const auto& def : du.getDefinitions())
    {
        variables.insert(def.variable);
        defBlocks[def.variable].insert(
            def.blockId);
    }

    for(const auto& use : du.getUses())
    {
        variables.insert(use.variable);
        useBlocks[use.variable].insert(
            use.blockId);
    }

    for(const auto& var : variables)
    {
        pde.defineVariable(var);

        int totalPaths =
            static_cast<int>(
                allPaths.size());

        for(int i = 0; i < totalPaths; i++)
        {
            pde.addExecutionPath(var);
        }

        int usedPathCount = 0;

        for(const auto& path : allPaths)
        {
            bool usedOnPath = false;

            for(int blockId : path)
            {
                if(useBlocks[var].count(
                       blockId) != 0)
                {
                    usedOnPath = true;
                    break;
                }
            }

            if(usedOnPath)
            {
                usedPathCount++;
            }
        }

        for(int i = 0; i < usedPathCount; i++)
        {
            pde.useVariable(var);
        }
    }
}

void CFGPDE::analyze()
{
    pde.printResults();
}
