#include "CFGCache.h"
#include "Phase8Optimizer.h"
#include <algorithm>
#include <iostream>

namespace Phase8
{

void CFGCache::initialize(const CFG& cfg)
{
    this->cfg = &cfg;
    clear();
}

const std::vector<int>& CFGCache::getReachableBlocks(int startBlock)
{
    // Check cache first
    auto it = traversalCache.find(startBlock);
    if(it != traversalCache.end())
    {
        stats.hits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second.reachableBlocks;
    }

    stats.misses++;
    BenchmarkState::instance().recordCacheMiss();

    // Compute and cache
    if(!cfg || !cfg->hasBlock(startBlock))
    {
        static const std::vector<int> empty;
        return empty;
    }

    TraversalResult result;
    std::vector<int> order;
    std::unordered_set<int> visited;
    std::unordered_set<int> onStack;

    dfsTraverse(startBlock, order, visited, onStack, result);

    stats.traversalsPerformed++;
    stats.nodesVisited += visited.size();

    result.reachableBlocks.assign(visited.begin(), visited.end());
    result.blocksInOrder = order;

    traversalCache[startBlock] = result;
    return traversalCache[startBlock].reachableBlocks;
}

const std::vector<int>& CFGCache::getBlocksInOrder(int startBlock)
{
    auto it = traversalCache.find(startBlock);
    if(it != traversalCache.end())
    {
        stats.hits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second.blocksInOrder;
    }

    // Trigger computation
    getReachableBlocks(startBlock);
    return traversalCache[startBlock].blocksInOrder;
}

bool CFGCache::isReachable(int fromBlock, int toBlock)
{
    auto key = std::make_pair(fromBlock, toBlock);
    auto it = reachabilityCache.find(key);
    if(it != reachabilityCache.end())
    {
        stats.hits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second;
    }

    stats.misses++;
    BenchmarkState::instance().recordCacheMiss();

    const auto& reachable = getReachableBlocks(fromBlock);
    bool result = std::find(reachable.begin(), reachable.end(), toBlock) != reachable.end();
    reachabilityCache[key] = result;
    return result;
}

const std::unordered_set<int>& CFGCache::getPredecessorClosure(int block)
{
    auto it = predecessorClosures.find(block);
    if(it != predecessorClosures.end())
    {
        stats.hits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second;
    }

    stats.misses++;
    BenchmarkState::instance().recordCacheMiss();

    std::unordered_set<int> closure;
    std::unordered_set<int> visited;
    computePredecessorClosure(block, closure, visited);
    predecessorClosures[block] = closure;
    return predecessorClosures[block];
}

const std::unordered_set<int>& CFGCache::getSuccessorClosure(int block)
{
    auto it = successorClosures.find(block);
    if(it != successorClosures.end())
    {
        stats.hits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second;
    }

    stats.misses++;
    BenchmarkState::instance().recordCacheMiss();

    std::unordered_set<int> closure;
    std::unordered_set<int> visited;
    computeSuccessorClosure(block, closure, visited);
    successorClosures[block] = closure;
    return successorClosures[block];
}

void CFGCache::invalidateRegion(const std::unordered_set<int>& changedBlocks)
{
    for(int block : changedBlocks)
    {
        invalidatedBlocks.insert(block);
        traversalCache.erase(block);
        predecessorClosures.erase(block);
        successorClosures.erase(block);
    }
}

void CFGCache::clear()
{
    traversalCache.clear();
    predecessorClosures.clear();
    successorClosures.clear();
    reachabilityCache.clear();
    invalidatedBlocks.clear();
}

void CFGCache::dfsTraverse(int block, std::vector<int>& order,
                           std::unordered_set<int>& visited,
                           std::unordered_set<int>& onStack,
                           TraversalResult& result)
{
    if(visited.count(block) != 0)
    {
        if(onStack.count(block) != 0)
        {
            result.hasCycles = true;
            result.backEdgeTargets.insert(block);
        }
        return;
    }

    visited.insert(block);
    onStack.insert(block);
    order.push_back(block);

    if(!cfg)
        return;

    const BasicBlock* bb = cfg->getBlock(block);
    if(!bb)
        return;

    for(int succ : bb->succs)
    {
        dfsTraverse(succ, order, visited, onStack, result);
    }

    onStack.erase(block);
}

void CFGCache::computeSuccessorClosure(int block, std::unordered_set<int>& closure,
                                      std::unordered_set<int>& visited)
{
    if(visited.count(block) != 0)
        return;

    visited.insert(block);
    closure.insert(block);

    if(!cfg)
        return;

    const BasicBlock* bb = cfg->getBlock(block);
    if(!bb)
        return;

    for(int succ : bb->succs)
    {
        computeSuccessorClosure(succ, closure, visited);
    }
}

void CFGCache::computePredecessorClosure(int block, std::unordered_set<int>& closure,
                                        std::unordered_set<int>& visited)
{
    if(visited.count(block) != 0)
        return;

    visited.insert(block);
    closure.insert(block);

    if(!cfg)
        return;

    const BasicBlock* bb = cfg->getBlock(block);
    if(!bb)
        return;

    for(int pred : bb->preds)
    {
        computePredecessorClosure(pred, closure, visited);
    }
}

} // namespace Phase8
