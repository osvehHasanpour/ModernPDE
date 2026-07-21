#pragma once

#include "CFG.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Phase8
{

/// Result of a CFG traversal from a specific block
struct TraversalResult
{
    std::vector<int> reachableBlocks;
    std::vector<int> blocksInOrder;
    std::unordered_set<int> backEdgeTargets;
    bool hasCycles = false;
    int visitCount = 0;
};

/// Cached CFG traversal engine with memoization
class CFGCache
{
public:
    CFGCache() = default;

    /// Initialize cache with a CFG
    void initialize(const CFG& cfg);

    /// Get reachable blocks from a starting block (cached)
    const std::vector<int>& getReachableBlocks(int startBlock);

    /// Get all blocks in DFS order (cached)
    const std::vector<int>& getBlocksInOrder(int startBlock);

    /// Check if block B is reachable from block A (cached)
    bool isReachable(int fromBlock, int toBlock);

    /// Get predecessor closure (cached): all blocks that can reach this block
    const std::unordered_set<int>& getPredecessorClosure(int block);

    /// Get successor closure (cached): all blocks reachable from this block
    const std::unordered_set<int>& getSuccessorClosure(int block);

    /// Mark a CFG region as changed (invalidate cache for affected blocks)
    void invalidateRegion(const std::unordered_set<int>& changedBlocks);

    /// Clear all caches
    void clear();

    /// Get cache statistics
    struct CacheStats
    {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t traversalsPerformed = 0;
        uint64_t nodesVisited = 0;

        double hitRate() const
        {
            uint64_t total = hits + misses;
            return total > 0 ? (100.0 * hits) / total : 0.0;
        }
    };

    const CacheStats& getStats() const { return stats; }

private:
    const CFG* cfg = nullptr;
    std::unordered_map<int, TraversalResult> traversalCache;
    std::unordered_map<int, std::unordered_set<int>> predecessorClosures;
    std::unordered_map<int, std::unordered_set<int>> successorClosures;
    std::unordered_map<std::pair<int, int>, bool, std::hash<std::pair<int, int>>> reachabilityCache;
    std::unordered_set<int> invalidatedBlocks;
    mutable CacheStats stats;

    /// Internal DFS traversal with cycle detection
    void dfsTraverse(int block, std::vector<int>& order,
                     std::unordered_set<int>& visited,
                     std::unordered_set<int>& onStack,
                     TraversalResult& result);

    /// Compute closure sets (all reachable or all that reach)
    void computeSuccessorClosure(int block, std::unordered_set<int>& closure,
                                std::unordered_set<int>& visited);
    void computePredecessorClosure(int block, std::unordered_set<int>& closure,
                                  std::unordered_set<int>& visited);
};

} // namespace Phase8
