/*
 * Lexer / Parser / CFGBuilder smoke tests on sample inputs.
 */

#include "CFGBuilder.h"
#include "Lexer.h"
#include "Parser.h"

#include <iostream>
#include <string>

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

struct SampleExpectation
{
    const char* path;
    std::size_t expectedFunctions;
};

static bool runSample(
    const SampleExpectation& sample)
{
    std::cout
        << "\n[ "
        << sample.path
        << " ]\n";

    ModernPDE::Lexer lexer;

    if(!lexer.tokenizeFile(sample.path))
    {
        CHECK(false, "lexer failed to tokenize file");
        return false;
    }

    CHECK(
        lexer.tokens().size() > 0,
        "token count > 0");

    ModernPDE::Parser parser(lexer.tokens());

    parser.parse();

    CHECK(
        parser.functionCount() ==
            sample.expectedFunctions,
        std::string("function count == ") +
        std::to_string(sample.expectedFunctions));

    CFGBuilder builder;

    CFG cfg = builder.build(parser.getRoot());

    CHECK(
        cfg.size() > 0,
        "CFG has at least one block");

    return true;
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  Lexer / Parser / CFG Test\n";

    std::cout
        << "================================================\n";

    const SampleExpectation samples[] = {
        { "tests/dead.cpp",              1 },
        { "tests/cfg.cpp",               1 },
        { "tests/oop.cpp",               1 },
        { "tests/partial.cpp",           1 },
        { "tests/recursion.cpp",         2 },
        { "tests/template.cpp",          1 },
        { "tests/mutual_recursive.cpp",  3 },
    };

    for(const auto& sample : samples)
    {
        runSample(sample);
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
