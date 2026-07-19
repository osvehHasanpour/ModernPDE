/*
 * Interprocedural Data-Flow Analysis regression tests.
 */

#include "CallGraph.h"
#include "InterproceduralDataFlow.h"

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

static Instruction makeAssign(
    int id,
    const std::string& result,
    const std::string& use = "")
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Assign;
    instruction.result = result;

    if(!use.empty())
    {
        instruction.uses.insert(use);
    }

    return instruction;
}

static Instruction makeCall(
    int id,
    const std::string& result)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Call;
    instruction.result = result;

    return instruction;
}

static Instruction makeReturn(
    int id,
    const std::string& value)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Return;
    instruction.uses.insert(value);

    return instruction;
}

static Instruction makeFieldStore(
    int id,
    const std::string& field,
    const std::string& value)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::FieldStore;
    instruction.result = field;
    instruction.uses.insert(value);

    return instruction;
}

static bool hasChain(
    const InterproceduralDataFlowAnalysis& analysis,
    const std::string& defFunction,
    const std::string& defVariable,
    const std::string& useFunction,
    const std::string& useVariable)
{
    for(const auto& chain : analysis.chains())
    {
        if(chain.def.function == defFunction &&
           chain.def.definition.variable == defVariable &&
           chain.use.function == useFunction &&
           chain.use.use.variable == useVariable)
        {
            return true;
        }
    }

    return false;
}

static FunctionIR buildMain()
{
    FunctionIR function;

    function.name = "main";
    function.instructions.push_back(
        makeAssign(1, "alpha"));
    function.instructions.push_back(
        makeAssign(2, "grid"));
    function.instructions.push_back(
        makeCall(3, "coeff"));
    function.instructions.push_back(
        makeCall(4, "grid"));

    return function;
}

static FunctionIR buildCoefficient()
{
    FunctionIR function;

    function.name = "coefficient";
    function.instructions.push_back(
        makeReturn(10, "alphaIn"));

    return function;
}

static FunctionIR buildUpdateGrid()
{
    FunctionIR function;

    function.name = "updateGrid";
    function.instructions.push_back(
        makeFieldStore(20, "gridIn.temperature", "coeffIn"));
    function.instructions.push_back(
        makeReturn(21, "gridIn"));

    return function;
}

static FunctionIR buildRecursiveHelper()
{
    FunctionIR function;

    function.name = "recursiveHelper";
    function.instructions.push_back(
        makeCall(30, "next"));
    function.instructions.push_back(
        makeReturn(31, "n"));

    return function;
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  Interprocedural Data-Flow Analysis Test\n";

    std::cout
        << "================================================\n";

    CallGraph graph;

    graph.addContextSensitiveEdge(
        "main",
        "coefficient",
        "CTX_SOLVER",
        "main:3");

    graph.addContextSensitiveEdge(
        "main",
        "updateGrid",
        "CTX_SOLVER",
        "main:4");

    graph.addContextSensitiveEdge(
        "recursiveHelper",
        "recursiveHelper",
        "CTX_REC",
        "recursive_call");

    std::vector<FunctionIR> functions;
    functions.push_back(buildMain());
    functions.push_back(buildCoefficient());
    functions.push_back(buildUpdateGrid());
    functions.push_back(buildRecursiveHelper());

    InterproceduralDataFlowAnalysis analysis;

    analysis.setFunctionParameters(
        "coefficient",
        {"alphaIn"});

    analysis.setFunctionParameters(
        "updateGrid",
        {"gridIn", "coeffIn"});

    analysis.setFunctionParameters(
        "recursiveHelper",
        {"n"});

    InterproceduralCallSite coeffCall;
    coeffCall.caller = "main";
    coeffCall.callee = "coefficient";
    coeffCall.context = "CTX_SOLVER";
    coeffCall.returnVariable = "coeff";
    coeffCall.arguments.push_back("alpha");
    coeffCall.line = 3;
    analysis.addCallSite(coeffCall);

    InterproceduralCallSite updateCall;
    updateCall.caller = "main";
    updateCall.callee = "updateGrid";
    updateCall.context = "CTX_SOLVER";
    updateCall.returnVariable = "grid";
    updateCall.arguments.push_back("grid");
    updateCall.arguments.push_back("coeff");
    updateCall.line = 4;
    analysis.addCallSite(updateCall);

    InterproceduralCallSite recursiveCall;
    recursiveCall.caller = "recursiveHelper";
    recursiveCall.callee = "recursiveHelper";
    recursiveCall.context = "CTX_REC";
    recursiveCall.returnVariable = "next";
    recursiveCall.arguments.push_back("n");
    recursiveCall.line = 30;
    analysis.addCallSite(recursiveCall);

    analysis.run(functions, graph);

    CHECK(
        analysis.validate(),
        "call sites validate against known functions and parameters");

    CHECK(
        hasChain(
            analysis,
            "main",
            "alpha",
            "coefficient",
            "alphaIn"),
        "main alpha reaches coefficient parameter");

    CHECK(
        hasChain(
            analysis,
            "main",
            "alpha",
            "main",
            "coeff"),
        "coefficient return reaches main coeff");

    CHECK(
        hasChain(
            analysis,
            "main",
            "alpha",
            "updateGrid",
            "coeffIn"),
        "returned coefficient reaches updateGrid parameter");

    CHECK(
        analysis.stats().recursiveSCCs == 1,
        "recursive SCC detected");

    CHECK(
        analysis.stats().convergenceIterations > 0,
        "fixed point iteration ran");

    CHECK(
        analysis.stats().pdeFieldLinks > 0,
        "PDE field definitions propagate across call boundary");

    CHECK(
        analysis.stats().interproceduralChains ==
            analysis.chains().size(),
        "chain statistics match result size");

    analysis.printStatistics();

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
