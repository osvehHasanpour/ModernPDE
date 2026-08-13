#include "Phase8Optimizer.h"
#include <iostream>

namespace Phase8
{

BenchmarkState& BenchmarkState::instance()
{
    static BenchmarkState instance;
    return instance;
}

void BenchmarkState::reset()
{
    metrics = BenchmarkMetrics();
}

void BenchmarkState::recordTraversal(int nodeCount, int edgeCount)
{
    metrics.cfgTraversalCount++;
    metrics.cfgNodesVisited += nodeCount;
    metrics.cfgEdgesTraversed += edgeCount;
}

void BenchmarkState::recordDUAnalysis(int defCount, int useCount, int chainCount)
{
    (void)useCount;

    metrics.definitionsProcessed += defCount;
    metrics.duChainsAnalyzed += chainCount;
}

void BenchmarkState::recordPathEnumeration(int pathCount)
{
    metrics.pathsEnumerated += pathCount;
}

void BenchmarkState::recordCacheHit()
{
    metrics.cacheHits++;
}

void BenchmarkState::recordCacheMiss()
{
    metrics.cacheMisses++;
}

void BenchmarkState::recordMemoizationHit()
{
    metrics.memoizationHits++;
}

void BenchmarkState::recordRedundantTraversal()
{
    metrics.redundantTraversalsAvoided++;
}

void BenchmarkState::recordWorksiteReduction()
{
    metrics.worksiteReductions++;
}

void BenchmarkState::recordEarlyTermination()
{
    metrics.earlyTerminations++;
}

void BenchmarkState::setStartTime()
{
    startTime = std::chrono::high_resolution_clock::now();
}

void BenchmarkState::setEndTime()
{
    endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    metrics.runtimeMs = duration.count();
}

MemoryTracker& MemoryTracker::instance()
{
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::recordAllocation(std::size_t bytes)
{
    currentMemory += bytes;
    if(currentMemory > peakMemory)
    {
        peakMemory = currentMemory;
    }
}

void MemoryTracker::recordDeallocation(std::size_t bytes)
{
    if(currentMemory >= bytes)
    {
        currentMemory -= bytes;
    }
}

} // namespace Phase8
