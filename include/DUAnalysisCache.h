#pragma once

#include "DUChain.h"
#include "CFG.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Phase8
{

/// Cached reaching definitions information
struct ReachingDefInfo
{
    int blockId = -1;
    std::unordered_set<int> inSet;      // Definitions reaching IN
    std::unordered_set<int> outSet;     // Definitions reaching OUT
    std::unordered_set<int> genSet;     // Definitions generated
    std::unordered_set<int> killSet;    // Definitions killed
    bool isValid = false;
};

/// Optimized DU analysis with caching and incrementality
class DUAnalysisCache
{
public:
    DUAnalysisCache() = default;

    /// Initialize with DU chains and CFG
    void initialize(const DUChainAnalysis& duAnalysis, const CFG& cfg);

    /// Get reaching definitions for a block (cached)
    const std::unordered_set<int>& getReachingDefinitions(int blockId);

    /// Get definitions that reach a specific use
    const std::vector<int>& getReachingDefsForUse(int useId);

    /// Check if a definition reaches a use (cached)
    bool definitionReaches(int defId, int useId);

    /// Mark blocks as changed and update incrementally
    void updateChangedBlocks(const std::unordered_set<int>& changedBlockIds);

    /// Get all DU chains (from initialization)
    const std::vector<DUChain>& getDUChains() const { return duChains; }

    /// Get analysis statistics
    struct AnalysisStats
    {
        uint64_t chainsAnalyzed = 0;
        uint64_t reachingDefsComputed = 0;
        uint64_t cacheHits = 0;
        uint64_t cacheMisses = 0;
        uint64_t incrementalUpdates = 0;

        double cacheHitRate() const
        {
            uint64_t total = cacheHits + cacheMisses;
            return total > 0 ? (100.0 * cacheHits) / total : 0.0;
        }
    };

    const AnalysisStats& getStats() const { return stats; }

private:
    const CFG* cfg = nullptr;
    std::vector<DUChain> duChains;
    std::vector<Definition> definitions;
    std::vector<Use> uses;

    std::unordered_map<int, ReachingDefInfo> blockReachingDefs;
    std::unordered_map<int, std::vector<int>> useToReachingDefs;
    std::unordered_set<int> invalidatedBlocks;
    mutable AnalysisStats stats;

    /// Compute reaching definitions using worklist algorithm
    void computeReachingDefinitions();

    /// Update reaching definitions for a single block
    void updateBlockReachingDefs(int blockId);

    /// Propagate changes through dependent blocks
    void propagateChanges(const std::unordered_set<int>& changedBlocks);
};

} // namespace Phase8
