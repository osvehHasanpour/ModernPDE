#pragma once

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <cstdint>

namespace Phase8
{

/// Benchmark metrics for Phase 7 vs Phase 8 comparison
struct BenchmarkMetrics
{
    // Timing
    uint64_t runtimeMs = 0;
    uint64_t userTimeMs = 0;
    uint64_t systemTimeMs = 0;

    // Memory
    uint64_t peakMemoryMB = 0;
    uint64_t allocatedBytesMB = 0;

    // CFG Traversals
    uint64_t cfgTraversalCount = 0;
    uint64_t cfgNodesVisited = 0;
    uint64_t cfgEdgesTraversed = 0;

    // DU Analysis
    uint64_t duChainsAnalyzed = 0;
    uint64_t reachingDefsComputed = 0;
    uint64_t definitionsProcessed = 0;

    // Path Enumeration
    uint64_t pathsEnumerated = 0;
    uint64_t lazyPathGenerations = 0;

    // Cache Statistics
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint64_t memoizationHits = 0;

    // Optimization Impact
    uint64_t redundantTraversalsAvoided = 0;
    uint64_t worksiteReductions = 0;
    uint64_t earlyTerminations = 0;

    double cacheHitRate() const
    {
        uint64_t total = cacheHits + cacheMisses;
        return total > 0 ? (100.0 * cacheHits) / total : 0.0;
    }

    double speedup(const BenchmarkMetrics& phase7) const
    {
        return phase7.runtimeMs > 0 ?
            static_cast<double>(phase7.runtimeMs) / runtimeMs : 0.0;
    }

    double memoryReduction(const BenchmarkMetrics& phase7) const
    {
        return phase7.peakMemoryMB > 0 ?
            (100.0 * (phase7.peakMemoryMB - peakMemoryMB)) / phase7.peakMemoryMB : 0.0;
    }
};

/// Global benchmark state for Phase 8
class BenchmarkState
{
public:
    static BenchmarkState& instance();

    void reset();
    void recordTraversal(int nodeCount, int edgeCount);
    void recordDUAnalysis(int defCount, int useCount, int chainCount);
    void recordPathEnumeration(int pathCount);
    void recordCacheHit();
    void recordCacheMiss();
    void recordMemoizationHit();
    void recordRedundantTraversal();
    void recordWorksiteReduction();
    void recordEarlyTermination();

    const BenchmarkMetrics& getMetrics() const { return metrics; }
    void setStartTime();
    void setEndTime();

private:
    BenchmarkState() = default;
    BenchmarkMetrics metrics;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
};

/// Memory tracking for peak memory usage
class MemoryTracker
{
public:
    static MemoryTracker& instance();

    void recordAllocation(std::size_t bytes);
    void recordDeallocation(std::size_t bytes);
    uint64_t peakMemoryBytes() const { return peakMemory; }
    uint64_t currentMemoryBytes() const { return currentMemory; }

private:
    MemoryTracker() = default;
    uint64_t currentMemory = 0;
    uint64_t peakMemory = 0;
};

} // namespace Phase8
