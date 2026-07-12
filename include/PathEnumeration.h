#pragma once

#include "CFG.h"

#include <cstddef>
#include <unordered_set>
#include <vector>

class PathEnumeration
{
public:

    void enumerate(
        const CFG& cfg,
        int startBlock);

    void print() const;

    std::size_t pathCount() const;

    const std::vector<
        std::vector<int>>&
    getPaths() const;

    std::size_t backEdgeCount() const;

private:

    void dfs(
        const CFG& cfg,
        int current,
        std::vector<int>& currentPath,
        std::unordered_set<int>& onPath);

    std::vector<
        std::vector<int>
    > paths;

    std::size_t backEdgeCount_ = 0;
};
