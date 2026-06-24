#pragma once

#include "CFG.h"

#include <vector>

class PathEnumeration
{
public:

    void enumerate(
        const CFG& cfg,
        int startBlock);

    void print() const;

private:

    void dfs(
        const CFG& cfg,
        int current,
        std::vector<int>& currentPath);

    std::vector<
        std::vector<int>
    > paths;
};