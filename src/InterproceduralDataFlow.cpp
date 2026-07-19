#include "InterproceduralDataFlow.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stack>

bool QualifiedDefinition::operator<(
    const QualifiedDefinition& other) const
{
    if(function != other.function)
    {
        return function < other.function;
    }

    if(definition.variable != other.definition.variable)
    {
        return definition.variable < other.definition.variable;
    }

    if(definition.line != other.definition.line)
    {
        return definition.line < other.definition.line;
    }

    if(definition.blockId != other.definition.blockId)
    {
        return definition.blockId < other.definition.blockId;
    }

    return definition.defId < other.definition.defId;
}

void InterproceduralDataFlowAnalysis::setFunctionParameters(
    const std::string& function,
    const std::vector<std::string>& parameters)
{
    parameters_[function] = parameters;
}

void InterproceduralDataFlowAnalysis::addCallSite(
    const InterproceduralCallSite& site)
{
    callSites_.push_back(site);
}

void InterproceduralDataFlowAnalysis::run(
    const std::vector<FunctionIR>& functions,
    const CallGraph& callGraph)
{
    (void)callGraph;

    functions_.clear();
    summaries_.clear();
    chains_.clear();
    chainKeys_.clear();
    graph_.clear();
    stats_ = InterproceduralStats();

    for(const auto& function : functions)
    {
        functions_[function.name] = function;
        summaries_[summaryKey(function.name, "")] =
            buildIntraSummary(function, "");
    }

    for(const auto& site : callSites_)
    {
        graph_[site.caller].push_back(site.callee);

        if(!site.context.empty() &&
           functions_.count(site.callee) != 0 &&
           summaries_.count(summaryKey(site.callee, site.context)) == 0)
        {
            summaries_[summaryKey(site.callee, site.context)] =
                buildIntraSummary(
                    functions_[site.callee],
                    site.context);
        }

        if(!site.context.empty() &&
           functions_.count(site.caller) != 0 &&
           summaries_.count(summaryKey(site.caller, site.context)) == 0)
        {
            summaries_[summaryKey(site.caller, site.context)] =
                buildIntraSummary(
                    functions_[site.caller],
                    site.context);
        }
    }

    stats_.functionsAnalyzed = summaries_.size();
    stats_.callSitesAnalyzed = callSites_.size();

    for(const auto& scc : computeSCCs())
    {
        if(scc.size() > 1)
        {
            stats_.recursiveSCCs++;
            continue;
        }

        const auto found = graph_.find(scc.front());

        if(found != graph_.end() &&
           std::find(
               found->second.begin(),
               found->second.end(),
               scc.front()) != found->second.end())
        {
            stats_.recursiveSCCs++;
        }
    }

    bool changed = true;

    while(changed)
    {
        changed = false;
        stats_.convergenceIterations++;

        for(const auto& site : callSites_)
        {
            if(propagateCall(site))
            {
                changed = true;
            }
        }
    }

    stats_.interproceduralChains = chains_.size();
}

bool InterproceduralDataFlowAnalysis::validate() const
{
    for(const auto& site : callSites_)
    {
        if(functions_.count(site.caller) == 0 ||
           functions_.count(site.callee) == 0)
        {
            return false;
        }

        if(site.arguments.size() >
           parametersFor(site.callee).size())
        {
            return false;
        }
    }

    return true;
}

void InterproceduralDataFlowAnalysis::printStatistics() const
{
    std::cout
        << "\nInterprocedural Data-Flow Statistics\n"
        << "Functions analyzed: "
        << stats_.functionsAnalyzed
        << "\nCall sites analyzed: "
        << stats_.callSitesAnalyzed
        << "\nPropagated definitions: "
        << stats_.propagatedDefinitions
        << "\nInterprocedural DU chains: "
        << stats_.interproceduralChains
        << "\nConvergence iterations: "
        << stats_.convergenceIterations
        << "\nRecursive SCCs: "
        << stats_.recursiveSCCs
        << "\nPDE field links: "
        << stats_.pdeFieldLinks
        << "\n";
}

const InterproceduralStats&
InterproceduralDataFlowAnalysis::stats() const
{
    return stats_;
}

const std::vector<InterproceduralDUChain>&
InterproceduralDataFlowAnalysis::chains() const
{
    return chains_;
}

const std::map<std::string, FunctionSummary>&
InterproceduralDataFlowAnalysis::summaries() const
{
    return summaries_;
}

FunctionSummary InterproceduralDataFlowAnalysis::buildIntraSummary(
    const FunctionIR& function,
    const std::string& context) const
{
    FunctionSummary summary;

    summary.function = function.name;
    summary.context = context;

    for(const auto& instruction : function.instructions)
    {
        for(const auto& use : instruction.uses)
        {
            if(instruction.type != InstType::Return)
            {
                continue;
            }

            summary.returnVariables.insert(use);

            const auto found =
                summary.reachingDefinitions.find(use);

            if(found != summary.reachingDefinitions.end())
            {
                summary.returnDefinitions.insert(
                    found->second.begin(),
                    found->second.end());
            }
        }

        if(instruction.result.empty() ||
           instruction.result[0] == '@' ||
           instruction.type == InstType::Return ||
           instruction.type == InstType::Call)
        {
            continue;
        }

        Definition definition;
        definition.variable = instruction.result;
        definition.blockId = 0;
        definition.line = instruction.id;
        definition.defId = instruction.id;

        QualifiedDefinition qualified;
        qualified.function = function.name;
        qualified.definition = definition;

        summary.definitions.insert(qualified);
        summary.reachingDefinitions[instruction.result].clear();
        summary.reachingDefinitions[instruction.result].insert(qualified);

        if(instruction.type == InstType::FieldStore)
        {
            summary.fieldDefinitions[instruction.result].insert(qualified);
        }
    }

    if(summary.returnDefinitions.empty())
    {
        const auto found =
            summary.reachingDefinitions.find("return");

        if(found != summary.reachingDefinitions.end())
        {
            summary.returnDefinitions.insert(
                found->second.begin(),
                found->second.end());
        }
    }

    return summary;
}

bool InterproceduralDataFlowAnalysis::propagateCall(
    const InterproceduralCallSite& site)
{
    bool changed = false;

    const std::string callerKey =
        summaryKey(site.caller, site.context);

    const std::string calleeKey =
        summaryKey(site.callee, site.context);

    auto callerFound = summaries_.find(callerKey);
    auto calleeFound = summaries_.find(calleeKey);

    if(callerFound == summaries_.end())
    {
        callerFound = summaries_.find(summaryKey(site.caller, ""));
    }

    if(calleeFound == summaries_.end())
    {
        calleeFound = summaries_.find(summaryKey(site.callee, ""));
    }

    if(callerFound == summaries_.end() ||
       calleeFound == summaries_.end())
    {
        return false;
    }

    FunctionSummary& caller = callerFound->second;
    FunctionSummary& callee = calleeFound->second;
    const auto& parameters = parametersFor(site.callee);

    for(std::size_t i = 0;
        i < site.arguments.size() && i < parameters.size();
        ++i)
    {
        const auto reaching =
            caller.reachingDefinitions.find(site.arguments[i]);

        if(reaching == caller.reachingDefinitions.end())
        {
            continue;
        }

        Use use;
        use.variable = parameters[i];
        use.blockId = 0;
        use.line = site.line;

        QualifiedUse qualifiedUse;
        qualifiedUse.function = site.callee;
        qualifiedUse.use = use;

        for(const auto& def : reaching->second)
        {
            changed |= addChain(
                def,
                qualifiedUse,
                site.context);
        }

        changed |= mergeDefinitions(
            callee.reachingDefinitions[parameters[i]],
            reaching->second);

        if(callee.returnVariables.count(parameters[i]) != 0)
        {
            changed |= mergeDefinitions(
                callee.returnDefinitions,
                callee.reachingDefinitions[parameters[i]]);
        }
    }

    if(!site.returnVariable.empty())
    {
        Use use;
        use.variable = site.returnVariable;
        use.blockId = 0;
        use.line = site.line;

        QualifiedUse qualifiedUse;
        qualifiedUse.function = site.caller;
        qualifiedUse.use = use;

        for(const auto& def : callee.returnDefinitions)
        {
            changed |= addChain(
                def,
                qualifiedUse,
                site.context);
        }

        changed |= mergeDefinitions(
            caller.reachingDefinitions[site.returnVariable],
            callee.returnDefinitions);
    }

    for(const auto& field : callee.fieldDefinitions)
    {
        std::size_t before =
            caller.fieldDefinitions[field.first].size();

        caller.fieldDefinitions[field.first].insert(
            field.second.begin(),
            field.second.end());

        if(caller.fieldDefinitions[field.first].size() != before)
        {
            changed = true;
            stats_.pdeFieldLinks +=
                caller.fieldDefinitions[field.first].size() - before;
        }
    }

    return changed;
}

std::vector<std::vector<std::string>>
InterproceduralDataFlowAnalysis::computeSCCs() const
{
    std::map<std::string, int> index;
    std::map<std::string, int> lowlink;
    std::set<std::string> onStack;
    std::stack<std::string> stack;
    std::vector<std::vector<std::string>> components;
    int nextIndex = 0;

    std::function<void(const std::string&)> visit =
        [&](const std::string& node)
    {
        index[node] = nextIndex;
        lowlink[node] = nextIndex;
        nextIndex++;
        stack.push(node);
        onStack.insert(node);

        const auto found = graph_.find(node);

        if(found != graph_.end())
        {
            for(const auto& next : found->second)
            {
                if(index.count(next) == 0)
                {
                    visit(next);
                    lowlink[node] =
                        std::min(
                            lowlink[node],
                            lowlink[next]);
                }
                else if(onStack.count(next) != 0)
                {
                    lowlink[node] =
                        std::min(
                            lowlink[node],
                            index[next]);
                }
            }
        }

        if(lowlink[node] != index[node])
        {
            return;
        }

        std::vector<std::string> component;

        while(!stack.empty())
        {
            const std::string current = stack.top();
            stack.pop();
            onStack.erase(current);
            component.push_back(current);

            if(current == node)
            {
                break;
            }
        }

        components.push_back(component);
    };

    for(const auto& function : functions_)
    {
        if(index.count(function.first) == 0)
        {
            visit(function.first);
        }
    }

    return components;
}

std::string InterproceduralDataFlowAnalysis::summaryKey(
    const std::string& function,
    const std::string& context) const
{
    if(context.empty())
    {
        return function;
    }

    return function + "#" + context;
}

bool InterproceduralDataFlowAnalysis::addChain(
    const QualifiedDefinition& def,
    const QualifiedUse& use,
    const std::string& context)
{
    const std::string key =
        def.function + ":" +
        def.definition.variable + ":" +
        std::to_string(def.definition.line) + "->" +
        use.function + ":" +
        use.use.variable + ":" +
        std::to_string(use.use.line) + "#" +
        context;

    if(chainKeys_.count(key) != 0)
    {
        return false;
    }

    chainKeys_.insert(key);

    InterproceduralDUChain chain;
    chain.def = def;
    chain.use = use;
    chain.context = context;
    chains_.push_back(chain);
    stats_.propagatedDefinitions++;

    return true;
}

bool InterproceduralDataFlowAnalysis::mergeDefinitions(
    std::set<QualifiedDefinition>& target,
    const std::set<QualifiedDefinition>& source)
{
    const std::size_t before = target.size();

    target.insert(
        source.begin(),
        source.end());

    return target.size() != before;
}

const std::vector<std::string>&
InterproceduralDataFlowAnalysis::parametersFor(
    const std::string& function) const
{
    static const std::vector<std::string> empty;

    const auto found = parameters_.find(function);

    if(found == parameters_.end())
    {
        return empty;
    }

    return found->second;
}
