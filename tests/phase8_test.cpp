#include <iostream>
#include <cassert>
#include <chrono>
#include "Phase8Optimizer.h"
#include "CFGCache.h"
#include "DUAnalysisCache.h"
#include "DominatorAnalysis.h"
#include "PathEnumerationLazy.h"
#include "CFG.h"
#include "DUChain.h"
#include "PDE.h"

using namespace Phase8;

void testCFGCache()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: CFG CACHE\n";
    std::cout << std::string(60, '=') << "\n";

    CFG cfg;
    int b0 = cfg.createBlock();
    int b1 = cfg.createBlock();
    int b2 = cfg.createBlock();
    int b3 = cfg.createBlock();

    cfg.addEdge(b0, b1);
    cfg.addEdge(b0, b2);
    cfg.addEdge(b1, b3);
    cfg.addEdge(b2, b3);

    CFGCache cache;
    cache.initialize(cfg);

    // Test 1: Get reachable blocks (should cache)
    const auto& reachable = cache.getReachableBlocks(b0);
    std::cout << "Reachable from block 0: ";
    for(int b : reachable)
        std::cout << b << " ";
    std::cout << "\n";
    assert(reachable.size() == 4);  // All blocks reachable from entry

    // Test 2: Cache hit on second call
    const auto& reachable2 = cache.getReachableBlocks(b0);
    assert(reachable2.size() == 4);
    std::cout << "Cache stats: " << cache.getStats().hits << " hits, "
              << cache.getStats().misses << " misses\n";
    assert(cache.getStats().hits > 0);  // Should have at least one hit

    // Test 3: Reachability query
    bool reachable_b0_b3 = cache.isReachable(b0, b3);
    std::cout << "Block 3 reachable from block 0: " << (reachable_b0_b3 ? "YES" : "NO") << "\n";
    assert(reachable_b0_b3);

    bool reachable_b3_b0 = cache.isReachable(b3, b0);
    std::cout << "Block 0 reachable from block 3: " << (reachable_b3_b0 ? "YES" : "NO") << "\n";
    assert(!reachable_b3_b0);  // b0 not reachable from b3 (DAG)

    std::cout << "✓ CFG Cache tests PASSED\n";
}

void testDominatorAnalysis()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: DOMINATOR ANALYSIS\n";
    std::cout << std::string(60, '=') << "\n";

    CFG cfg;
    int entry = cfg.createBlock();
    int b1 = cfg.createBlock();
    int b2 = cfg.createBlock();
    int b3 = cfg.createBlock();

    cfg.addEdge(entry, b1);
    cfg.addEdge(entry, b2);
    cfg.addEdge(b1, b3);
    cfg.addEdge(b2, b3);

    DominatorAnalysis dom;
    dom.compute(cfg, entry);

    // Test: entry dominates everything
    assert(dom.dominates(entry, entry));
    assert(dom.dominates(entry, b1));
    assert(dom.dominates(entry, b2));
    assert(dom.dominates(entry, b3));

    // Test: b1 does not dominate b2
    assert(!dom.dominates(b1, b2));

    // Test: b1 dominates b3 on left path only (not strict dominator)
    // In this diamond, neither b1 nor b2 strictly dominates b3
    std::cout << "Dominator relationships verified\n";

    std::cout << "✓ Dominator Analysis tests PASSED\n";
}

void testPathEnumerationLazy()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: LAZY PATH ENUMERATION\n";
    std::cout << std::string(60, '=') << "\n";

    CFG cfg;
    int entry = cfg.createBlock();
    int b1 = cfg.createBlock();
    int b2 = cfg.createBlock();
    int b3 = cfg.createBlock();

    cfg.addEdge(entry, b1);
    cfg.addEdge(entry, b2);
    cfg.addEdge(b1, b3);
    cfg.addEdge(b2, b3);

    PathEnumerationLazy pathEnum;
    pathEnum.initialize(cfg, entry);

    // Test: Enumerate paths
    std::size_t pathCount = 0;
    for(auto it = pathEnum.begin(); it != pathEnum.end(); ++it)
    {
        pathCount++;
        std::cout << "Path " << pathCount << ": ";
        for(int block : *it)
        {
            std::cout << block << " ";
        }
        std::cout << "\n";
    }
    std::cout << "Total paths: " << pathCount << "\n";
    assert(pathCount == 2);  // Diamond has 2 paths

    std::cout << "✓ Lazy Path Enumeration tests PASSED\n";
}

void testMemoryTracking()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: MEMORY TRACKING\n";
    std::cout << std::string(60, '=') << "\n";

    MemoryTracker& tracker = MemoryTracker::instance();

    uint64_t before = tracker.currentMemoryBytes();
    tracker.recordAllocation(1024);
    uint64_t after = tracker.currentMemoryBytes();

    std::cout << "Before: " << before << " bytes\n";
    std::cout << "After allocation: " << after << " bytes\n";
    std::cout << "Peak memory: " << tracker.peakMemoryBytes() << " bytes\n";

    assert(after == before + 1024);
    assert(tracker.peakMemoryBytes() >= after);

    std::cout << "✓ Memory Tracking tests PASSED\n";
}

void testBenchmarkMetrics()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: BENCHMARK METRICS\n";
    std::cout << std::string(60, '=') << "\n";

    BenchmarkState& state = BenchmarkState::instance();
    state.reset();

    state.setStartTime();
    state.recordTraversal(10, 20);
    state.recordDUAnalysis(5, 10, 8);
    state.recordCacheHit();
    state.recordCacheHit();
    state.recordCacheMiss();
    state.setEndTime();

    const auto& metrics = state.getMetrics();

    std::cout << "Runtime: " << metrics.runtimeMs << " ms\n";
    std::cout << "CFG Traversals: " << metrics.cfgTraversalCount << "\n";
    std::cout << "DU Chains: " << metrics.duChainsAnalyzed << "\n";
    std::cout << "Cache Hit Rate: " << metrics.cacheHitRate() << "%\n";

    assert(metrics.cfgNodesVisited == 10);
    assert(metrics.duChainsAnalyzed == 8);
    assert(metrics.cacheHitRate() > 60.0);

    std::cout << "✓ Benchmark Metrics tests PASSED\n";
}

void testPDEClassification()
{
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: PDE CLASSIFICATION (Correctness Check)\n";
    std::cout << std::string(60, '=') << "\n";

    PDE pde;

    // Test variable: dead (defined but never used)
    pde.defineVariable("deadVar");
    for(int i = 0; i < 10; ++i)
        pde.addExecutionPath("deadVar");
    // No uses

    std::string classification = pde.classify("deadVar");
    std::cout << "deadVar classification: " << classification << "\n";
    assert(classification == "DEAD");

    // Test variable: mostly dead (< 25% usage)
    pde.defineVariable("mostlyDeadVar");
    for(int i = 0; i < 10; ++i)
        pde.addExecutionPath("mostlyDeadVar");
    pde.useVariable("mostlyDeadVar");
    // 1/10 = 10% usage

    classification = pde.classify("mostlyDeadVar");
    std::cout << "mostlyDeadVar classification: " << classification << "\n";
    assert(classification == "MOSTLY DEAD");

    // Test variable: partially dead (25-75% usage)
    pde.defineVariable("partiallyDeadVar");
    for(int i = 0; i < 10; ++i)
        pde.addExecutionPath("partiallyDeadVar");
    for(int i = 0; i < 5; ++i)
        pde.useVariable("partiallyDeadVar");
    // 5/10 = 50% usage

    classification = pde.classify("partiallyDeadVar");
    std::cout << "partiallyDeadVar classification: " << classification << "\n";
    assert(classification == "PARTIALLY DEAD");

    // Test variable: live (100% usage)
    pde.defineVariable("liveVar");
    for(int i = 0; i < 10; ++i)
    {
        pde.addExecutionPath("liveVar");
        pde.useVariable("liveVar");
    }
    // 10/10 = 100% usage

    classification = pde.classify("liveVar");
    std::cout << "liveVar classification: " << classification << "\n";
    assert(classification == "LIVE");

    std::cout << "✓ PDE Classification tests PASSED\n";
}

int main()
{
    std::cout << "\n" << std::string(70, '#') << "\n";
    std::cout << "#" << std::string(68, ' ') << "#\n";
    std::cout << "#" << std::string(15, ' ') << "PHASE 8 OPTIMIZATION TEST SUITE" << std::string(22, ' ') << "#\n";
    std::cout << "#" << std::string(68, ' ') << "#\n";
    std::cout << std::string(70, '#') << "\n\n";

    try
    {
        testCFGCache();
        testDominatorAnalysis();
        testPathEnumerationLazy();
        testMemoryTracking();
        testBenchmarkMetrics();
        testPDEClassification();

        std::cout << "\n" << std::string(70, '#') << "\n";
        std::cout << "#" << std::string(68, ' ') << "#\n";
        std::cout << "#" << std::string(20, ' ') << "ALL TESTS PASSED ✓" << std::string(30, ' ') << "#\n";
        std::cout << "#" << std::string(68, ' ') << "#\n";
        std::cout << std::string(70, '#') << "\n\n";

        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "\nTEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
