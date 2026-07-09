#include "PathEnumeration.h"

#include <iostream>

std::size_t PathEnumeration::pathCount() const
{
    return paths.size();
}

void PathEnumeration::enumerate(
    const CFG& cfg,
    int startBlock)
{
    paths.clear();

    std::vector<int> currentPath;

    dfs(
        cfg,
        startBlock,
        currentPath);
}

void PathEnumeration::dfs(
    const CFG& cfg,
    int current,
    std::vector<int>& currentPath)
{
    currentPath.push_back(current);

    const BasicBlock* block =
        cfg.getBlock(current);

    if(block == nullptr)
    {
        currentPath.pop_back();
        return;
    }

    if(block->succs.empty())
    {
        paths.push_back(
            currentPath);

        currentPath.pop_back();
        return;
    }

    for(int next : block->succs)
    {
        dfs(
            cfg,
            next,
            currentPath);
    }

    currentPath.pop_back();
}

void PathEnumeration::print() const
{
    std::cout
    << "\n====================\n";

    std::cout
    << "PATH ENUMERATION\n";

    std::cout
    << "====================\n";

    int index = 1;

    for(const auto& path : paths)
    {
        std::cout
        << "Path "
        << index++
        << ": ";

        for(size_t i = 0;
            i < path.size();
            i++)
        {
            std::cout
            << path[i];

            if(i + 1 < path.size())
            {
                std::cout
                << " -> ";
            }
        }

        std::cout
        << "\n";
    }
}