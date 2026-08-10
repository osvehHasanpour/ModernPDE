#include "PathEnumeration.h"

#include <iostream>

std::size_t PathEnumeration::pathCount() const
{
    return paths.size();
}

const std::vector<
    std::vector<int>>&
PathEnumeration::getPaths() const
{
    return paths;
}

std::size_t PathEnumeration::backEdgeCount() const
{
    return backEdgeCount_;
}

void PathEnumeration::enumerate(
    const CFG& cfg,
    int startBlock)
{
    paths.clear();
    backEdgeCount_ = 0;

    if(!cfg.hasBlock(startBlock))
    {
        return;
    }

    std::vector<int> currentPath;
    std::unordered_set<int> onPath;

    dfs(
        cfg,
        startBlock,
        currentPath,
        onPath);
}

void PathEnumeration::dfs(
    const CFG& cfg,
    int current,
    std::vector<int>& currentPath,
    std::unordered_set<int>& onPath)
{
    if(onPath.count(current) != 0)
    {
        backEdgeCount_++;
        return;
    }

    currentPath.push_back(current);
    onPath.insert(current);

    const BasicBlock* block =
        cfg.getBlock(current);

    if(block == nullptr)
    {
        currentPath.pop_back();
        onPath.erase(current);
        return;
    }

    if(block->succs.empty())
    {
        paths.push_back(currentPath);
    }
    else
    {
        for(int next : block->succs)
        {
            dfs(
                cfg,
                next,
                currentPath,
                onPath);
        }
    }

    currentPath.pop_back();
    onPath.erase(current);
}

void PathEnumeration::print() const
{
    std::cout
    << "\n====================\n";

    std::cout
    << "PATH ENUMERATION\n";

    std::cout
    << "====================\n";

    std::cout
    << "Paths      : "
    << paths.size()
    << "\n";

    std::cout
    << "Back edges : "
    << backEdgeCount_
    << "\n";

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
