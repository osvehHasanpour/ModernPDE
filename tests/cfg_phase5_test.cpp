/*
 * Phase 5 integration tests: CFG, DU chains, paths, validation, PDE.
 * Assertions are structural (not hardcoded CFG shapes).
 */

#include "CFGBuilder.h"
#include "CFGPDE.h"
#include "CFGStatistics.h"
#include "CFGValidation.h"
#include "DUChain.h"
#include "Lexer.h"
#include "Parser.h"
#include "PathEnumeration.h"
#include "PDE.h"

#include <iostream>
#include <string>
#include <vector>

using namespace ModernPDE;

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

struct Phase5Case
{
    const char* path;

    std::size_t minBlocks;

    std::size_t minPaths;

    bool expectValidCfg;

    bool expectDuChains;
};

static std::vector<CFGFunctionBounds>
toFunctionBounds(
    const CFGBuilder& builder)
{
    std::vector<CFGFunctionBounds> bounds;

    for(const auto& function :
        builder.functions())
    {
        CFGFunctionBounds item;
        item.name       = function.name;
        item.entryBlock = function.entryBlock;
        item.exitBlock  = function.exitBlock;
        bounds.push_back(item);
    }

    return bounds;
}

static void runPhase5Case(
    const Phase5Case& testCase)
{
    std::cout
        << "\n[ "
        << testCase.path
        << " ]\n";

    Lexer lexer;

    CHECK(
        lexer.tokenizeFile(testCase.path),
        "lexer tokenized input");

    Parser parser(lexer.tokens());
    parser.parse();

    CFGBuilder builder;
    CFG cfg = builder.build(parser.getRoot());

    CHECK(
        cfg.size() >= testCase.minBlocks,
        "CFG has enough basic blocks");

    auto fnBounds = toFunctionBounds(builder);

    CHECK(
        !fnBounds.empty(),
        "at least one function region recorded");

    PathEnumeration paths;
    paths.enumerate(
        cfg,
        builder.entryBlock());

    CHECK(
        paths.pathCount() >= testCase.minPaths,
        "enumerated enough execution paths");

    CFGValidation::Report report =
        CFGValidation::validate(
            cfg,
            builder.entryBlock(),
            fnBounds);

    CHECK(
        report.valid == testCase.expectValidCfg,
        "CFG validation expectation");

    for(const auto& fn : report.functions)
    {
        CHECK(
            fn.validEntry,
            std::string(fn.name)
            + " has valid entry block");

        CHECK(
            fn.validExit,
            std::string(fn.name)
            + " has valid exit block");
    }

    auto fnStats =
        CFGStatistics::computePerFunction(
            cfg,
            fnBounds,
            paths);

    CHECK(
        fnStats.size() == fnBounds.size(),
        "per-function statistics count");

    for(const auto& stats : fnStats)
    {
        CHECK(
            stats.blocks >= 2,
            std::string(stats.functionName)
            + " has entry and exit blocks");
    }

    DUChainAnalysis du;
    du.buildFromCFG(
        cfg,
        builder.blockStatements());

    if(testCase.expectDuChains)
    {
        CHECK(
            du.chainCount() > 0,
            "DU chains derived from CFG");
    }

    CFGPDE cfgPde;
    cfgPde.analyzeFromCFG(
        cfg,
        paths,
        du,
        builder.entryBlock());

    CHECK(
        !du.getDefinitions().empty() ||
        !du.getUses().empty() ||
        !testCase.expectDuChains,
        "DU analysis collected program facts");
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  Phase 5 CFG Pipeline Test\n";

    std::cout
        << "================================================\n";

    const Phase5Case cases[] =
    {
        { "tests/cfg.cpp",              5, 1, true,  true  },
        { "tests/cfg_while.cpp",        5, 1, true,  true  },
        { "tests/cfg_for.cpp",          6, 1, true,  true  },
        { "tests/cfg_nested_if.cpp",    8, 1, true,  true  },
        { "tests/cfg_do_while.cpp",     5, 1, true,  true  },
        { "tests/cfg_break_continue.cpp", 6, 1, true, true },
        { "tests/cfg_switch.cpp",       5, 1, true,  true  },
        { "tests/cfg_multi_return.cpp", 5, 1, true,  false },
        { "tests/recursion.cpp",        5, 1, true,  false },
        { "tests/mutual_recursive.cpp", 5, 1, true,  false },
        { "tests/oop.cpp",              3, 1, true,  false },
    };

    for(const auto& testCase : cases)
    {
        runPhase5Case(testCase);
    }

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
