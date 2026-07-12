/*
 * CFG path enumeration and DU-chain tests.
 */

#include "CFG.h"
#include "DUChain.h"
#include "PathEnumeration.h"

#include <iostream>

static int g_total  = 0;
static int g_passes = 0;
static int g_fails  = 0;

#define CHECK(cond, msg) do { \
    g_total++; \
    if(cond) { g_passes++; } \
    else { \
        g_fails++; \
        std::cout << "  FAIL  " << (msg) << "\n"; \
    } \
} while(0)

static CFG buildDiamondCfg()
{
    CFG cfg;

    int start =
        cfg.createBlock();

    int thenBlock =
        cfg.createBlock();

    int elseBlock =
        cfg.createBlock();

    int mergeBlock =
        cfg.createBlock();

    cfg.addEdge(start, thenBlock);
    cfg.addEdge(start, elseBlock);
    cfg.addEdge(thenBlock, mergeBlock);
    cfg.addEdge(elseBlock, mergeBlock);

    return cfg;
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  CFG Path + DU Chain Test\n";

    std::cout
        << "================================================\n";

    CFG cfg = buildDiamondCfg();

    CHECK(
        cfg.size() == 4,
        "diamond CFG has 4 blocks");

    PathEnumeration paths;

    paths.enumerate(cfg, 0);

    CHECK(
        paths.pathCount() == 2,
        "diamond CFG has 2 paths from entry");

    DUChainAnalysis du;

    du.addDefinition("x", 1);
    du.addUse("x", 2);

    du.addDefinition("x", 3);
    du.addUse("x", 4);

    du.addDefinition("y", 5);
    du.addUse("y", 6);

    du.addDefinition("z", 7);
    du.addUse("z", 10);

    du.buildChains();

    CHECK(
        du.chainCount() == 4,
        "DU chains count == 4");

    std::cout
        << "\n================================================\n";

    std::cout
        << "  Total : "
        << g_total
        << "  Pass  : "
        << g_passes
        << "  Fail  : "
        << g_fails
        << "\n";

    if(g_fails == 0)
    {
        std::cout
            << "  RESULT : ALL TESTS PASSED\n";
    }
    else
    {
        std::cout
            << "  RESULT : "
            << g_fails
            << " TEST(S) FAILED\n";
    }

    std::cout
        << "================================================\n";

    return g_fails != 0 ? 1 : 0;
}
