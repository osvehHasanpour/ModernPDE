#pragma once

#include "CallGraph.h"
#include "DUChain.h"
#include "IR.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

struct QualifiedDefinition
{
    std::string function;

    Definition definition;

    bool operator<(
        const QualifiedDefinition& other) const;
};

struct QualifiedUse
{
    std::string function;

    Use use;
};

struct InterproceduralDUChain
{
    QualifiedDefinition def;

    QualifiedUse use;

    std::string context;
};

struct InterproceduralCallSite
{
    std::string caller;

    std::string callee;

    std::string context;

    std::string returnVariable;

    std::vector<std::string> arguments;

    int line = 0;
};

struct FunctionSummary
{
    std::string function;

    std::string context;

    std::set<QualifiedDefinition> definitions;

    std::set<QualifiedDefinition> returnDefinitions;

    std::set<std::string> returnVariables;

    std::map<
        std::string,
        std::set<QualifiedDefinition>> reachingDefinitions;

    std::map<
        std::string,
        std::set<QualifiedDefinition>> fieldDefinitions;
};

struct InterproceduralStats
{
    std::size_t functionsAnalyzed = 0;

    std::size_t callSitesAnalyzed = 0;

    std::size_t propagatedDefinitions = 0;

    std::size_t interproceduralChains = 0;

    std::size_t convergenceIterations = 0;

    std::size_t recursiveSCCs = 0;

    std::size_t pdeFieldLinks = 0;
};

class InterproceduralDataFlowAnalysis
{
public:

    void setFunctionParameters(
        const std::string& function,
        const std::vector<std::string>& parameters);

    void addCallSite(
        const InterproceduralCallSite& site);

    void run(
        const std::vector<FunctionIR>& functions,
        const CallGraph& callGraph);

    bool validate() const;

    void printStatistics() const;

    const InterproceduralStats& stats() const;

    const std::vector<InterproceduralDUChain>& chains() const;

    const std::map<std::string, FunctionSummary>& summaries() const;

private:

    FunctionSummary buildIntraSummary(
        const FunctionIR& function,
        const std::string& context) const;

    bool propagateCall(
        const InterproceduralCallSite& site);

    std::vector<std::vector<std::string>> computeSCCs() const;

    std::string summaryKey(
        const std::string& function,
        const std::string& context) const;

    bool addChain(
        const QualifiedDefinition& def,
        const QualifiedUse& use,
        const std::string& context);

    bool mergeDefinitions(
        std::set<QualifiedDefinition>& target,
        const std::set<QualifiedDefinition>& source);

    const std::vector<std::string>& parametersFor(
        const std::string& function) const;

private:

    std::map<std::string, FunctionIR> functions_;

    std::map<std::string, std::vector<std::string>> parameters_;

    std::vector<InterproceduralCallSite> callSites_;

    std::map<std::string, FunctionSummary> summaries_;

    std::vector<InterproceduralDUChain> chains_;

    std::set<std::string> chainKeys_;

    std::map<std::string, std::vector<std::string>> graph_;

    InterproceduralStats stats_;
};
