#pragma once

#include <string>
#include <unordered_map>

#include "CFG.h"
#include "DUChain.h"
#include "PathEnumeration.h"
#include "PDE.h"

class CFGPDE
{
public:

    void registerVariable(
        const std::string& name,
        int totalPaths,
        int usedPaths);

    void analyzeFromCFG(
        const CFG& cfg,
        const PathEnumeration& paths,
        const DUChainAnalysis& du,
        int entryBlock = 0);

    void analyze();

private:

    PDE pde;
};
