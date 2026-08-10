#include "CFGStatistics.h"
#include "PathEnumeration.h"

#include <functional>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>

namespace
{

std::unordered_set<int> forwardReachable(
    const CFG& cfg,
    int start)
{
    std::unordered_set<int> reachable;
    std::queue<int> worklist;

    if(!cfg.hasBlock(start))
    {
        return reachable;
    }

    worklist.push(start);
    reachable.insert(start);

    while(!worklist.empty())
    {
        int current = worklist.front();
        worklist.pop();

        const BasicBlock* block =
            cfg.getBlock(current);

        if(block == nullptr)
        {
            continue;
        }

        for(int succ : block->succs)
        {
            if(reachable.insert(succ).second)
            {
                worklist.push(succ);
            }
        }
    }

    return reachable;
}

std::unordered_set<int> backwardReachable(
    const CFG& cfg,
    int end)
{
    std::unordered_set<int> reachable;
    std::queue<int> worklist;

    if(!cfg.hasBlock(end))
    {
        return reachable;
    }

    worklist.push(end);
    reachable.insert(end);

    while(!worklist.empty())
    {
        int current = worklist.front();
        worklist.pop();

        const BasicBlock* block =
            cfg.getBlock(current);

        if(block == nullptr)
        {
            continue;
        }

        for(int pred : block->preds)
        {
            if(reachable.insert(pred).second)
            {
                worklist.push(pred);
            }
        }
    }

    return reachable;
}

CFGStatistics::Stats computeRegionStats(
    const CFG& cfg,
    const std::unordered_set<int>& region,
    int entryBlock,
    std::size_t pathCount)
{
    CFGStatistics::Stats stats;

    if(region.empty())
    {
        return stats;
    }

    stats.blocks = region.size();

    double succSum = 0.0;
    double sizeSum = 0.0;

    for(int blockId : region)
    {
        const BasicBlock* block =
            cfg.getBlock(blockId);

        if(block == nullptr)
        {
            continue;
        }

        for(int succ : block->succs)
        {
            if(region.count(succ) != 0)
            {
                stats.edges++;
            }
        }

        succSum +=
            static_cast<double>(
                block->succs.size());

        sizeSum +=
            static_cast<double>(
                block->instructions.size());
    }

    stats.cyclomaticComplexity =
        static_cast<int>(stats.edges)
        - static_cast<int>(stats.blocks)
        + 2;

    stats.avgSuccessors =
        succSum /
        static_cast<double>(stats.blocks);

    stats.avgBlockSize =
        sizeSum /
        static_cast<double>(stats.blocks);

    stats.executionPaths = pathCount;

    if(!cfg.hasBlock(entryBlock))
    {
        return stats;
    }

    std::function<void(
        int,
        int,
        std::unordered_set<int>&)> dfs;

    dfs = [&](
        int current,
        int depth,
        std::unordered_set<int>& onPath)
    {
        if(region.count(current) == 0)
        {
            return;
        }

        if(onPath.count(current) != 0)
        {
            return;
        }

        onPath.insert(current);

        if(depth > stats.maxDepth)
        {
            stats.maxDepth = depth;
        }

        const BasicBlock* block =
            cfg.getBlock(current);

        if(block == nullptr)
        {
            onPath.erase(current);
            return;
        }

        for(int succ : block->succs)
        {
            dfs(
                succ,
                depth + 1,
                onPath);
        }

        onPath.erase(current);
    };

    std::unordered_set<int> onPath;
    dfs(entryBlock, 0, onPath);

    return stats;
}

}

CFGStatistics::Stats CFGStatistics::compute(
    const CFG& cfg,
    int entryBlock,
    std::size_t pathCount)
{
    std::unordered_set<int> region;

    for(const auto& block : cfg.getBlocks())
    {
        region.insert(block.id);
    }

    return computeRegionStats(
        cfg,
        region,
        entryBlock,
        pathCount);
}

std::vector<CFGStatistics::FunctionStats>
CFGStatistics::computePerFunction(
    const CFG& cfg,
    const std::vector<CFGFunctionBounds>& functions,
    const PathEnumeration& paths)
{
    std::vector<FunctionStats> result;

    const auto& allPaths =
        paths.getPaths();

    for(const auto& function : functions)
    {
        FunctionStats fnStats;
        fnStats.functionName = function.name;
        fnStats.entryBlock   = function.entryBlock;
        fnStats.exitBlock    = function.exitBlock;

        auto forward =
            forwardReachable(
                cfg,
                function.entryBlock);

        auto backward =
            backwardReachable(
                cfg,
                function.exitBlock);

        std::unordered_set<int> region;

        for(int blockId : forward)
        {
            if(backward.count(blockId) != 0)
            {
                region.insert(blockId);
            }
        }

        std::size_t fnPathCount = 0;

        for(const auto& path : allPaths)
        {
            bool touchesEntry = false;

            for(int blockId : path)
            {
                if(blockId == function.entryBlock)
                {
                    touchesEntry = true;
                    break;
                }
            }

            if(touchesEntry)
            {
                fnPathCount++;
            }
        }

        Stats base =
            computeRegionStats(
                cfg,
                region,
                function.entryBlock,
                fnPathCount);

        static_cast<Stats&>(fnStats) = base;

        result.push_back(fnStats);
    }

    return result;
}

void CFGStatistics::print(
    const Stats& stats)
{
    std::cout
    << "\n====================\n";

    std::cout
    << "CFG STATISTICS\n";

    std::cout
    << "====================\n";

    std::cout
    << "Blocks               : "
    << stats.blocks
    << "\n";

    std::cout
    << "Edges                : "
    << stats.edges
    << "\n";

    std::cout
    << "Cyclomatic complexity: "
    << stats.cyclomaticComplexity
    << "\n";

    std::cout
    << "Maximum CFG depth    : "
    << stats.maxDepth
    << "\n";

    std::cout
    << "Average successors   : "
    << stats.avgSuccessors
    << "\n";

    std::cout
    << "Average block size   : "
    << stats.avgBlockSize
    << "\n";

    std::cout
    << "Execution paths      : "
    << stats.executionPaths
    << "\n";
}

void CFGStatistics::printPerFunction(
    const std::vector<FunctionStats>& stats)
{
    std::cout
    << "\n====================\n";

    std::cout
    << "PER-FUNCTION CFG STATS\n";

    std::cout
    << "====================\n";

    for(const auto& fn : stats)
    {
        std::cout
        << "\nFunction : "
        << fn.functionName
        << "\n";

        std::cout
        << "  Entry block        : "
        << fn.entryBlock
        << "\n";

        std::cout
        << "  Exit block         : "
        << fn.exitBlock
        << "\n";

        std::cout
        << "  Blocks             : "
        << fn.blocks
        << "\n";

        std::cout
        << "  Edges              : "
        << fn.edges
        << "\n";

        std::cout
        << "  Cyclomatic         : "
        << fn.cyclomaticComplexity
        << "\n";

        std::cout
        << "  Max depth          : "
        << fn.maxDepth
        << "\n";

        std::cout
        << "  Avg successors     : "
        << fn.avgSuccessors
        << "\n";

        std::cout
        << "  Avg block size     : "
        << fn.avgBlockSize
        << "\n";

        std::cout
        << "  Execution paths    : "
        << fn.executionPaths
        << "\n";
    }
}
