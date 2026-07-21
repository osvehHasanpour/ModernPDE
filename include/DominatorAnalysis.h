#pragma once

#include "CFG.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Phase8
{

/// Dominator tree node
struct DominatorNode
{
    int blockId = -1;
    int immediatelyDominates = -1;  // IDom
    std::vector<int> dominatedBy;   // Blocks that dominate this
    std::vector<int> dominates;     // Blocks dominated by this
};

/// Dominator analysis with incremental updates
class DominatorAnalysis
{
public:
    DominatorAnalysis() = default;

    /// Compute dominators using iterative algorithm
    void compute(const CFG& cfg, int entryBlock);

    /// Check if block A dominates block B
    bool dominates(int a, int b) const;

    /// Get immediate dominator
    int getImmediateDominator(int blockId) const;

    /// Get all blocks dominated by block
    const std::vector<int>& getDominatedBlocks(int blockId) const;

    /// Get all blocks that dominate block
    const std::vector<int>& getDominatingBlocks(int blockId) const;

    /// Get dominance frontier (for SSA)
    const std::unordered_set<int>& getDominanceFrontier(int blockId) const;

    /// Prune paths: returns blocks that are on all paths to target
    std::vector<int> getMustPathBlocks(int target) const;

    /// Get analysis statistics
    struct Stats
    {
        uint64_t iterationsNeeded = 0;
        uint64_t pathsPruned = 0;
    };

    const Stats& getStats() const { return stats; }

private:
    const CFG* cfg = nullptr;
    std::unordered_map<int, DominatorNode> domTree;
    std::unordered_map<int, std::unordered_set<int>> dominanceSetCache;
    std::unordered_map<int, std::unordered_set<int>> dominanceFrontierCache;
    mutable Stats stats;

    /// Iterative dominance computation
    void iterativeComputation(int entryBlock);

    /// Build dominance frontiers
    void buildDominanceFrontiers();
};

} // namespace Phase8
