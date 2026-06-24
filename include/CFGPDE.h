#pragma once

#include <string>
#include <unordered_map>

#include "CFG.h"
#include "PDE.h"

class CFGPDE
{
public:

    void registerVariable(
        const std::string& name,
        int totalPaths,
        int usedPaths);

    void analyze();

private:

    PDE pde;
};