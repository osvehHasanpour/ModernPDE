/*
 * CallGraph reachability and recursion tests.
 */

#include "CallGraph.h"

#include <iostream>
#include <string>

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

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  CallGraph Test\n";

    std::cout
        << "================================================\n";

    CallGraph graph;

    graph.addContextSensitiveEdge(
        "main",
        "loadAssets",
        "CTX_GAME",
        "site1");

    graph.addContextSensitiveEdge(
        "loadAssets",
        "loadTextures",
        "CTX_GAME",
        "site2");

    graph.addContextSensitiveEdge(
        "loadTextures",
        "uploadGPU",
        "CTX_GAME",
        "site3");

    graph.addContextSensitiveEdge(
        "main",
        "loadLevel",
        "CTX_LEVEL",
        "site4");

    graph.addContextSensitiveEdge(
        "loadLevel",
        "spawnNPC",
        "CTX_LEVEL",
        "site5");

    graph.addContextSensitiveEdge(
        "spawnNPC",
        "AI",
        "CTX_LEVEL",
        "site6");

    graph.addContextSensitiveEdge(
        "factorial",
        "factorial",
        "CTX_REC",
        "recursive_call");

    graph.addContextSensitiveEdge(
        "even",
        "odd",
        "CTX_MR",
        "site7");

    graph.addContextSensitiveEdge(
        "odd",
        "even",
        "CTX_MR",
        "site8");

    CHECK(
        graph.isReachable(
            "main",
            "uploadGPU",
            "CTX_GAME"),
        "main reaches uploadGPU in CTX_GAME");

    CHECK(
        !graph.isReachable(
            "main",
            "uploadGPU",
            "CTX_LEVEL"),
        "uploadGPU not reachable in CTX_LEVEL");

    CHECK(
        graph.isReachable(
            "main",
            "AI",
            "CTX_LEVEL"),
        "main reaches AI in CTX_LEVEL");

    CHECK(
        graph.isRecursiveFunction(
            "factorial",
            "CTX_REC"),
        "factorial is recursive");

    CHECK(
        graph.hasMutualRecursion(
            "even",
            "odd",
            "CTX_MR"),
        "even <-> odd mutual recursion");

    auto traversal =
        graph.traverseFrom(
            "main",
            "CTX_LEVEL");

    CHECK(
        !traversal.empty(),
        "CTX_LEVEL traversal non-empty");

    bool sawAI = false;

    for(const auto& node : traversal)
    {
        if(node.find("AI") != std::string::npos)
        {
            sawAI = true;
            break;
        }
    }

    CHECK(
        sawAI,
        "traversal includes AI");

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
