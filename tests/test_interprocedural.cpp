/*
 * Phase 7 Interprocedural Data-Flow Analysis tests.
 *
 * This file uses a tiny GoogleTest-compatible harness so the tests keep the
 * familiar TEST/EXPECT/RUN_ALL_TESTS shape without adding external
 * dependencies to the ModernPDE test build.
 */

#include "CFG.h"
#include "CallGraph.h"
#include "DUChain.h"
#include "FieldSensitiveAnalysis.h"
#include "InterproceduralDataFlow.h"
#include "IR.h"

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace testing
{

struct TestInfo
{
    std::string suite;
    std::string name;
    std::function<void()> body;
};

struct TestState
{
    int assertions = 0;
    int failures = 0;
    std::string currentSuite;
    std::string currentName;
};

static std::vector<TestInfo>& registry()
{
    static std::vector<TestInfo> tests;
    return tests;
}

static TestState& state()
{
    static TestState value;
    return value;
}

struct Registrar
{
    Registrar(
        const std::string& suite,
        const std::string& name,
        std::function<void()> body)
    {
        registry().push_back({suite, name, body});
    }
};

template <typename T>
std::string toString(
    const T& value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

static void expectTrue(
    bool condition,
    const std::string& expression,
    const char* file,
    int line)
{
    state().assertions++;

    if(condition)
    {
        std::cout
            << "    PASS "
            << expression
            << "\n";
        return;
    }

    state().failures++;

    std::cout
        << "    FAIL "
        << expression
        << " at "
        << file
        << ":"
        << line
        << "\n";
}

template <typename Left, typename Right>
void expectEq(
    const Left& left,
    const Right& right,
    const std::string& leftExpr,
    const std::string& rightExpr,
    const char* file,
    int line)
{
    state().assertions++;

    if(left == right)
    {
        std::cout
            << "    PASS "
            << leftExpr
            << " == "
            << rightExpr
            << " ("
            << toString(left)
            << ")\n";
        return;
    }

    state().failures++;

    std::cout
        << "    FAIL "
        << leftExpr
        << " == "
        << rightExpr
        << " at "
        << file
        << ":"
        << line
        << " actual="
        << toString(left)
        << " expected="
        << toString(right)
        << "\n";
}

static int InitGoogleTest(
    int*,
    char**)
{
    return 0;
}

static int RUN_ALL_TESTS()
{
    int failedTests = 0;
    const int totalTests =
        static_cast<int>(registry().size());

    std::cout
        << "================================================\n"
        << "  ModernPDE Phase 7 Test Summary\n"
        << "================================================\n";

    for(const auto& test : registry())
    {
        state().currentSuite = test.suite;
        state().currentName = test.name;
        const int failuresBefore = state().failures;

        std::cout
            << "\n[ RUN      ] "
            << test.suite
            << "."
            << test.name
            << "\n";

        test.body();

        if(state().failures == failuresBefore)
        {
            std::cout
                << "[       OK ] "
                << test.suite
                << "."
                << test.name
                << "\n";
        }
        else
        {
            failedTests++;
            std::cout
                << "[  FAILED  ] "
                << test.suite
                << "."
                << test.name
                << "\n";
        }
    }

    std::cout
        << "\n================================================\n"
        << "  Tests      : "
        << totalTests
        << "\n"
        << "  Assertions : "
        << state().assertions
        << "\n"
        << "  Failures   : "
        << state().failures
        << "\n"
        << "  Result     : "
        << (failedTests == 0 ? "ALL TESTS PASSED" : "FAILURES DETECTED")
        << "\n"
        << "================================================\n";

    return failedTests == 0 ? 0 : 1;
}

}

#define TEST(Suite, Name) \
    static void Suite##_##Name##_Body(); \
    static testing::Registrar Suite##_##Name##_registrar( \
        #Suite, \
        #Name, \
        Suite##_##Name##_Body); \
    static void Suite##_##Name##_Body()

#define EXPECT_TRUE(expr) \
    testing::expectTrue( \
        static_cast<bool>(expr), \
        #expr, \
        __FILE__, \
        __LINE__)

#define EXPECT_FALSE(expr) \
    testing::expectTrue( \
        !static_cast<bool>(expr), \
        "!(" #expr ")", \
        __FILE__, \
        __LINE__)

#define EXPECT_EQ(left, right) \
    testing::expectEq( \
        (left), \
        (right), \
        #left, \
        #right, \
        __FILE__, \
        __LINE__)

namespace
{

Instruction makeAssign(
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

Instruction makeCall(
    int id,
    const std::string& result)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Call;
    instruction.result = result;

    return instruction;
}

Instruction makeReturn(
    int id,
    const std::string& value)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Return;
    instruction.uses.insert(value);

    return instruction;
}

Instruction makeAlloc(
    int id,
    const std::string& result,
    const std::string& site)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Alloc;
    instruction.result = result;
    instruction.uses.insert(site);

    return instruction;
}

Instruction makeCopy(
    int id,
    const std::string& result,
    const std::string& source)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::Copy;
    instruction.result = result;
    instruction.uses.insert(source);

    return instruction;
}

Instruction makeFieldStore(
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

Instruction makeFieldLoad(
    int id,
    const std::string& result,
    const std::string& field)
{
    Instruction instruction;

    instruction.id = id;
    instruction.type = InstType::FieldLoad;
    instruction.result = result;
    instruction.uses.insert(field);

    return instruction;
}

FunctionIR heatMain()
{
    FunctionIR function;

    function.name = "heat_main";
    function.instructions.push_back(makeAssign(1, "dt"));
    function.instructions.push_back(makeAssign(2, "dx"));
    function.instructions.push_back(makeAssign(3, "grid"));
    function.instructions.push_back(makeCall(4, "alpha"));
    function.instructions.push_back(makeCall(5, "grid"));

    return function;
}

FunctionIR computeAlpha()
{
    FunctionIR function;

    function.name = "compute_alpha";
    function.instructions.push_back(makeReturn(10, "dtIn"));

    return function;
}

FunctionIR updateHeatGrid()
{
    FunctionIR function;

    function.name = "update_heat_grid";
    function.instructions.push_back(
        makeFieldStore(20, "gridIn.temperature", "alphaIn"));
    function.instructions.push_back(
        makeFieldStore(21, "gridIn.boundary", "dxIn"));
    function.instructions.push_back(makeReturn(22, "gridIn"));

    return function;
}

FunctionIR recursiveResidual()
{
    FunctionIR function;

    function.name = "recursive_residual";
    function.instructions.push_back(makeCall(30, "nextResidual"));
    function.instructions.push_back(makeReturn(31, "residual"));

    return function;
}

FunctionIR contextDriver()
{
    FunctionIR function;

    function.name = "context_driver";
    function.instructions.push_back(makeAssign(40, "coarse_dt"));
    function.instructions.push_back(makeAssign(41, "fine_dt"));
    function.instructions.push_back(makeCall(42, "coarse_alpha"));
    function.instructions.push_back(makeCall(43, "fine_alpha"));

    return function;
}

FunctionIR identityCoefficient()
{
    FunctionIR function;

    function.name = "identity_coefficient";
    function.instructions.push_back(makeReturn(50, "input_dt"));

    return function;
}

void printFunction(
    const FunctionIR& function)
{
    std::cout
        << "  Function "
        << function.name
        << "\n";

    for(const auto& instruction : function.instructions)
    {
        std::cout
            << "    id="
            << instruction.id
            << " result="
            << (instruction.result.empty() ? "<none>" : instruction.result)
            << " uses={";

        bool first = true;

        for(const auto& use : instruction.uses)
        {
            if(!first)
            {
                std::cout << ", ";
            }

            std::cout << use;
            first = false;
        }

        std::cout << "}\n";
    }
}

void printChains(
    const InterproceduralDataFlowAnalysis& analysis)
{
    std::cout
        << "  Interprocedural DU Chains\n";

    for(const auto& chain : analysis.chains())
    {
        std::cout
            << "    Def "
            << chain.def.function
            << "."
            << chain.def.definition.variable
            << "@"
            << chain.def.definition.line
            << " -> Use "
            << chain.use.function
            << "."
            << chain.use.use.variable
            << "@"
            << chain.use.use.line
            << " context="
            << (chain.context.empty() ? "<global>" : chain.context)
            << "\n";
    }
}

void printMappings(
    const std::vector<InterproceduralCallSite>& sites,
    const std::map<std::string, std::vector<std::string>>& parameters)
{
    std::cout
        << "  Parameter/Return Mappings\n";

    for(const auto& site : sites)
    {
        std::cout
            << "    "
            << site.caller
            << " -> "
            << site.callee
            << " context="
            << site.context
            << " line="
            << site.line
            << "\n";

        const auto found = parameters.find(site.callee);

        for(std::size_t i = 0; i < site.arguments.size(); ++i)
        {
            std::cout
                << "      arg "
                << i
                << ": "
                << site.arguments[i]
                << " -> ";

            if(found != parameters.end() &&
               i < found->second.size())
            {
                std::cout << found->second[i];
            }
            else
            {
                std::cout << "<unmapped>";
            }

            std::cout << "\n";
        }

        std::cout
            << "      return -> "
            << (site.returnVariable.empty() ?
                "<ignored>" :
                site.returnVariable)
            << "\n";
    }
}

void printSummaries(
    const InterproceduralDataFlowAnalysis& analysis)
{
    std::cout
        << "  Function Summaries\n";

    for(const auto& entry : analysis.summaries())
    {
        const FunctionSummary& summary = entry.second;

        std::cout
            << "    "
            << entry.first
            << " defs="
            << summary.definitions.size()
            << " returns="
            << summary.returnDefinitions.size()
            << " reaching-vars="
            << summary.reachingDefinitions.size()
            << " field-vars="
            << summary.fieldDefinitions.size()
            << "\n";

        for(const auto& reaching : summary.reachingDefinitions)
        {
            std::cout
                << "      reaching "
                << reaching.first
                << " <- ";

            bool first = true;

            for(const auto& def : reaching.second)
            {
                if(!first)
                {
                    std::cout << ", ";
                }

                std::cout
                    << def.function
                    << "."
                    << def.definition.variable
                    << "@"
                    << def.definition.line;
                first = false;
            }

            std::cout << "\n";
        }
    }
}

void printStats(
    const InterproceduralDataFlowAnalysis& analysis)
{
    const InterproceduralStats& stats = analysis.stats();

    std::cout
        << "  Summary Stats\n"
        << "    functionsAnalyzed="
        << stats.functionsAnalyzed
        << "\n"
        << "    callSitesAnalyzed="
        << stats.callSitesAnalyzed
        << "\n"
        << "    propagatedDefinitions="
        << stats.propagatedDefinitions
        << "\n"
        << "    interproceduralChains="
        << stats.interproceduralChains
        << "\n"
        << "    convergenceIterations="
        << stats.convergenceIterations
        << "\n"
        << "    recursiveSCCs="
        << stats.recursiveSCCs
        << "\n"
        << "    pdeFieldLinks="
        << stats.pdeFieldLinks
        << "\n";
}

bool hasChain(
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

bool summaryHasField(
    const InterproceduralDataFlowAnalysis& analysis,
    const std::string& summary,
    const std::string& field)
{
    const auto found = analysis.summaries().find(summary);

    if(found == analysis.summaries().end())
    {
        return false;
    }

    return found->second.fieldDefinitions.count(field) != 0;
}

bool summaryReaches(
    const InterproceduralDataFlowAnalysis& analysis,
    const std::string& summary,
    const std::string& variable,
    const std::string& defFunction,
    const std::string& defVariable)
{
    const auto found = analysis.summaries().find(summary);

    if(found == analysis.summaries().end())
    {
        return false;
    }

    const auto reaching =
        found->second.reachingDefinitions.find(variable);

    if(reaching == found->second.reachingDefinitions.end())
    {
        return false;
    }

    for(const auto& def : reaching->second)
    {
        if(def.function == defFunction &&
           def.definition.variable == defVariable)
        {
            return true;
        }
    }

    return false;
}

InterproceduralCallSite makeSite(
    const std::string& caller,
    const std::string& callee,
    const std::string& context,
    const std::string& result,
    const std::vector<std::string>& args,
    int line)
{
    InterproceduralCallSite site;

    site.caller = caller;
    site.callee = callee;
    site.context = context;
    site.returnVariable = result;
    site.arguments = args;
    site.line = line;

    return site;
}

}

TEST(Phase1To6Regression, CFGCallGraphDUAndFieldSensitiveBasics)
{
    std::cout
        << "  Phase 1-6 baseline before Phase 7\n";

    CFG cfg;
    const int entry = cfg.createBlock();
    const int body = cfg.createBlock();
    const int exit = cfg.createBlock();

    cfg.addEdge(entry, body);
    cfg.addEdge(body, exit);

    std::cout
        << "  CFG blocks="
        << cfg.size()
        << " edges="
        << cfg.edgeCount()
        << " path: "
        << entry
        << " -> "
        << body
        << " -> "
        << exit
        << "\n";

    EXPECT_EQ(cfg.size(), static_cast<std::size_t>(3));
    EXPECT_EQ(cfg.edgeCount(), static_cast<std::size_t>(2));

    CallGraph graph;
    graph.addContextSensitiveEdge(
        "heat_main",
        "compute_alpha",
        "CTX_HEAT",
        "line4");
    graph.addContextSensitiveEdge(
        "heat_main",
        "update_heat_grid",
        "CTX_HEAT",
        "line5");

    std::cout
        << "  CallGraph reachability before interprocedural pass\n"
        << "    heat_main -> compute_alpha: "
        << graph.isReachable("heat_main", "compute_alpha", "CTX_HEAT")
        << "\n"
        << "    heat_main -> update_heat_grid: "
        << graph.isReachable("heat_main", "update_heat_grid", "CTX_HEAT")
        << "\n";

    EXPECT_TRUE(
        graph.isReachable(
            "heat_main",
            "compute_alpha",
            "CTX_HEAT"));
    EXPECT_TRUE(
        graph.isReachable(
            "heat_main",
            "update_heat_grid",
            "CTX_HEAT"));

    DUChainAnalysis du;

    du.addDefinition("temperature", 1);
    du.addDefinition("laplacian", 2);
    du.addUse("temperature", 3);
    du.addUse("laplacian", 4);
    du.buildChains();

    std::cout
        << "  Intra DU before Phase 7 chains="
        << du.chainCount()
        << "\n";

    for(const auto& chain : du.getChains())
    {
        std::cout
            << "    Def "
            << chain.def.variable
            << "@"
            << chain.def.line
            << " -> Use "
            << chain.use.variable
            << "@"
            << chain.use.line
            << "\n";
    }

    EXPECT_EQ(du.chainCount(), static_cast<std::size_t>(2));

    FunctionIR fieldFunction;
    fieldFunction.name = "field_phase6";
    fieldFunction.instructions.push_back(
        makeAlloc(1, "grid", "Grid#heat"));
    fieldFunction.instructions.push_back(
        makeCopy(2, "aliasGrid", "grid"));
    fieldFunction.instructions.push_back(
        makeFieldStore(3, "grid.temperature", "hot"));
    fieldFunction.instructions.push_back(
        makeFieldLoad(4, "sample", "aliasGrid.temperature"));

    FieldSensitiveAnalysis fieldAnalysis;
    fieldAnalysis.run(fieldFunction);

    std::cout
        << "  FieldSensitive before Phase 7 objects="
        << fieldAnalysis.objects().size()
        << " pointsToVars="
        << fieldAnalysis.pointsTo().size()
        << " fieldReads="
        << fieldAnalysis.fieldReads().size()
        << "\n";

    EXPECT_TRUE(fieldAnalysis.pointsTo().count("grid") != 0);
    EXPECT_TRUE(fieldAnalysis.pointsTo().count("aliasGrid") != 0);
    EXPECT_EQ(fieldAnalysis.fieldReads().size(), static_cast<std::size_t>(1));
}

TEST(Phase7Interprocedural, HeatEquationCallChainPropagatesDefinitions)
{
    FunctionIR main = heatMain();
    FunctionIR alpha = computeAlpha();
    FunctionIR update = updateHeatGrid();

    std::cout
        << "  Before Phase 7: local IR only\n";
    printFunction(main);
    printFunction(alpha);
    printFunction(update);

    CallGraph graph;
    graph.addContextSensitiveEdge(
        "heat_main",
        "compute_alpha",
        "CTX_HEAT",
        "heat_main:4");
    graph.addContextSensitiveEdge(
        "heat_main",
        "update_heat_grid",
        "CTX_HEAT",
        "heat_main:5");

    InterproceduralDataFlowAnalysis analysis;
    std::map<std::string, std::vector<std::string>> parameters;
    std::vector<InterproceduralCallSite> sites;

    parameters["compute_alpha"] = {"dtIn", "dxIn"};
    parameters["update_heat_grid"] = {"gridIn", "alphaIn", "dxIn"};

    analysis.setFunctionParameters(
        "compute_alpha",
        parameters["compute_alpha"]);
    analysis.setFunctionParameters(
        "update_heat_grid",
        parameters["update_heat_grid"]);

    sites.push_back(
        makeSite(
            "heat_main",
            "compute_alpha",
            "CTX_HEAT",
            "alpha",
            {"dt", "dx"},
            4));
    sites.push_back(
        makeSite(
            "heat_main",
            "update_heat_grid",
            "CTX_HEAT",
            "grid",
            {"grid", "alpha", "dx"},
            5));

    printMappings(sites, parameters);

    for(const auto& site : sites)
    {
        analysis.addCallSite(site);
    }

    analysis.run({main, alpha, update}, graph);

    std::cout
        << "  After Phase 7: propagated results\n";
    printChains(analysis);
    printSummaries(analysis);
    printStats(analysis);

    EXPECT_TRUE(analysis.validate());
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "dt",
            "compute_alpha",
            "dtIn"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "dt",
            "heat_main",
            "alpha"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "dt",
            "update_heat_grid",
            "alphaIn"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "grid",
            "update_heat_grid",
            "gridIn"));
    EXPECT_EQ(analysis.stats().callSitesAnalyzed, static_cast<std::size_t>(2));
    EXPECT_TRUE(analysis.stats().interproceduralChains >= 5);
}

TEST(Phase7Interprocedural, RecursiveSolverConvergesWithSCC)
{
    FunctionIR driver;
    driver.name = "nonlinear_driver";
    driver.instructions.push_back(makeAssign(1, "residual0"));
    driver.instructions.push_back(makeCall(2, "residualFinal"));

    FunctionIR residual = recursiveResidual();

    std::cout
        << "  Before Phase 7 recursion case\n";
    printFunction(driver);
    printFunction(residual);

    CallGraph graph;
    graph.addContextSensitiveEdge(
        "nonlinear_driver",
        "recursive_residual",
        "CTX_NEWTON",
        "driver:2");
    graph.addContextSensitiveEdge(
        "recursive_residual",
        "recursive_residual",
        "CTX_NEWTON",
        "recursive_call");

    InterproceduralDataFlowAnalysis analysis;
    std::map<std::string, std::vector<std::string>> parameters;
    std::vector<InterproceduralCallSite> sites;

    parameters["recursive_residual"] = {"residual"};

    analysis.setFunctionParameters(
        "recursive_residual",
        parameters["recursive_residual"]);

    sites.push_back(
        makeSite(
            "nonlinear_driver",
            "recursive_residual",
            "CTX_NEWTON",
            "residualFinal",
            {"residual0"},
            2));
    sites.push_back(
        makeSite(
            "recursive_residual",
            "recursive_residual",
            "CTX_NEWTON",
            "nextResidual",
            {"residual"},
            30));

    printMappings(sites, parameters);

    for(const auto& site : sites)
    {
        analysis.addCallSite(site);
    }

    analysis.run({driver, residual}, graph);

    std::cout
        << "  After Phase 7 recursion fixed point\n";
    printChains(analysis);
    printSummaries(analysis);
    printStats(analysis);

    EXPECT_TRUE(analysis.validate());
    EXPECT_EQ(analysis.stats().recursiveSCCs, static_cast<std::size_t>(1));
    EXPECT_TRUE(analysis.stats().convergenceIterations >= 2);
    EXPECT_TRUE(
        hasChain(
            analysis,
            "nonlinear_driver",
            "residual0",
            "recursive_residual",
            "residual"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "nonlinear_driver",
            "residual0",
            "nonlinear_driver",
            "residualFinal"));
}

TEST(Phase7Interprocedural, ContextSensitiveSummariesStaySeparated)
{
    FunctionIR driver = contextDriver();
    FunctionIR coefficient = identityCoefficient();

    std::cout
        << "  Before Phase 7 context-sensitive coefficient calls\n";
    printFunction(driver);
    printFunction(coefficient);

    CallGraph graph;
    graph.addContextSensitiveEdge(
        "context_driver",
        "identity_coefficient",
        "CTX_COARSE",
        "driver:42");
    graph.addContextSensitiveEdge(
        "context_driver",
        "identity_coefficient",
        "CTX_FINE",
        "driver:43");

    InterproceduralDataFlowAnalysis analysis;
    std::map<std::string, std::vector<std::string>> parameters;
    std::vector<InterproceduralCallSite> sites;

    parameters["identity_coefficient"] = {"input_dt"};

    analysis.setFunctionParameters(
        "identity_coefficient",
        parameters["identity_coefficient"]);

    sites.push_back(
        makeSite(
            "context_driver",
            "identity_coefficient",
            "CTX_COARSE",
            "coarse_alpha",
            {"coarse_dt"},
            42));
    sites.push_back(
        makeSite(
            "context_driver",
            "identity_coefficient",
            "CTX_FINE",
            "fine_alpha",
            {"fine_dt"},
            43));

    printMappings(sites, parameters);

    for(const auto& site : sites)
    {
        analysis.addCallSite(site);
    }

    analysis.run({driver, coefficient}, graph);

    std::cout
        << "  After Phase 7 context-sensitive summaries\n";
    printChains(analysis);
    printSummaries(analysis);
    printStats(analysis);

    EXPECT_TRUE(analysis.validate());
    EXPECT_TRUE(
        hasChain(
            analysis,
            "context_driver",
            "coarse_dt",
            "context_driver",
            "coarse_alpha"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "context_driver",
            "fine_dt",
            "context_driver",
            "fine_alpha"));
    EXPECT_TRUE(
        analysis.summaries().count(
            "identity_coefficient#CTX_COARSE") != 0);
    EXPECT_TRUE(
        analysis.summaries().count(
            "identity_coefficient#CTX_FINE") != 0);
    EXPECT_TRUE(
        summaryReaches(
            analysis,
            "identity_coefficient#CTX_COARSE",
            "input_dt",
            "context_driver",
            "coarse_dt"));
    EXPECT_FALSE(
        summaryReaches(
            analysis,
            "identity_coefficient#CTX_COARSE",
            "input_dt",
            "context_driver",
            "fine_dt"));
    EXPECT_TRUE(
        summaryReaches(
            analysis,
            "identity_coefficient#CTX_FINE",
            "input_dt",
            "context_driver",
            "fine_dt"));
    EXPECT_FALSE(
        summaryReaches(
            analysis,
            "identity_coefficient#CTX_FINE",
            "input_dt",
            "context_driver",
            "coarse_dt"));
}

TEST(Phase7Interprocedural, PDEFieldPropagationAcrossGridHelpers)
{
    FunctionIR main = heatMain();
    FunctionIR update = updateHeatGrid();

    std::cout
        << "  Before Phase 7 PDE field propagation\n";
    printFunction(main);
    printFunction(update);

    CallGraph graph;
    graph.addContextSensitiveEdge(
        "heat_main",
        "update_heat_grid",
        "CTX_FIELDS",
        "heat_main:5");

    InterproceduralDataFlowAnalysis analysis;
    std::map<std::string, std::vector<std::string>> parameters;
    std::vector<InterproceduralCallSite> sites;

    parameters["update_heat_grid"] = {"gridIn", "alphaIn", "dxIn"};

    analysis.setFunctionParameters(
        "update_heat_grid",
        parameters["update_heat_grid"]);

    sites.push_back(
        makeSite(
            "heat_main",
            "update_heat_grid",
            "CTX_FIELDS",
            "grid",
            {"grid", "dt", "dx"},
            5));

    printMappings(sites, parameters);

    for(const auto& site : sites)
    {
        analysis.addCallSite(site);
    }

    analysis.run({main, update}, graph);

    std::cout
        << "  After Phase 7 PDE field propagation\n";
    printChains(analysis);
    printSummaries(analysis);
    printStats(analysis);

    EXPECT_TRUE(analysis.validate());
    EXPECT_TRUE(analysis.stats().pdeFieldLinks >= 2);
    EXPECT_TRUE(
        summaryHasField(
            analysis,
            "heat_main#CTX_FIELDS",
            "gridIn.temperature"));
    EXPECT_TRUE(
        summaryHasField(
            analysis,
            "heat_main#CTX_FIELDS",
            "gridIn.boundary"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "dt",
            "update_heat_grid",
            "alphaIn"));
    EXPECT_TRUE(
        hasChain(
            analysis,
            "heat_main",
            "dx",
            "update_heat_grid",
            "dxIn"));
}

int main(
    int argc,
    char** argv)
{
    testing::InitGoogleTest(
        &argc,
        argv);

    return testing::RUN_ALL_TESTS();
}
