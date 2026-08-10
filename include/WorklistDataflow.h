#pragma once

#include "CFG.h"
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Phase8
{

/// Worklist entry for dataflow propagation
struct WorklistEntry
{
    int blockId = -1;
    int priority = 0;  // Higher priority = process first

    bool operator<(const WorklistEntry& other) const
    {
        return priority < other.priority;  // Min-heap
    }
};

/// Sparse dataflow fact propagation
template<typename FactType>
class SparsePropagation
{
public:
    using TransferFunc = std::function<FactType(int, const FactType&)>;
    using MeetFunc = std::function<FactType(const std::vector<FactType>&)>;

    /// Initialize with CFG and initial facts
    void initialize(const CFG& cfg, const std::unordered_map<int, FactType>& initialFacts);

    /// Set the transfer function for a block
    void setTransferFunction(int blockId, const TransferFunc& fn);

    /// Set the meet operation
    void setMeetOperation(const MeetFunc& fn);

    /// Run dataflow analysis until fixed point
    void solve();

    /// Get the dataflow fact for a block
    const FactType& getFact(int blockId) const;

    /// Get changed blocks
    const std::unordered_set<int>& getChangedBlocks() const { return changedBlocks; }

    /// Statistics
    struct Stats
    {
        uint64_t iterations = 0;
        uint64_t worksiteSize = 0;
        uint64_t factsUpdated = 0;
    };

    const Stats& getStats() const { return stats; }

private:
    const CFG* cfg = nullptr;
    std::unordered_map<int, FactType> facts;
    std::unordered_map<int, TransferFunc> transferFunctions;
    MeetFunc meetOperation;
    std::priority_queue<WorklistEntry> worklist;
    std::unordered_set<int> changedBlocks;
    mutable Stats stats;
};

/// Worklist-based fixed-point dataflow engine
class WorklistDataflow
{
public:
    WorklistDataflow(const CFG& cfg) : cfg(cfg) {}

    /// Initialize with entry block
    void initialize(int entryBlock);

    /// Add block to worklist
    void addToWorklist(int blockId, int priority = 0);

    /// Process next item from worklist
    bool processNext();

    /// Check if worklist is empty
    bool isWorklistEmpty() const;

    /// Saturate worklist until fixed point
    void saturate();

    /// Get worklist size
    std::size_t worklistSize() const { return worklist.size(); }

    /// Statistics
    struct Stats
    {
        uint64_t iterations = 0;
        uint64_t itemsProcessed = 0;
        uint64_t maxWorklistSize = 0;
    };

    const Stats& getStats() const { return stats; }

private:
    const CFG& cfg;
    std::priority_queue<WorklistEntry> worklist;
    std::unordered_set<int> onWorklist;
    mutable Stats stats;
};

} // namespace Phase8
