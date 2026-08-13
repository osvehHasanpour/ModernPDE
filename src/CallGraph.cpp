#include "CallGraph.h"

#include <cstddef>
#include <iostream>
#include <set>

void CallGraph::registerFunction(
    const std::string& functionName)
{
    functions.insert(functionName);
}

void CallGraph::addEdge(
    const std::string& caller,
    const std::string& callee)
{
    registerFunction(caller);
    registerFunction(callee);

    graph[caller].push_back(callee);
}

void CallGraph::addContextSensitiveEdge(
    const std::string& caller,
    const std::string& callee,
    const std::string& context,
    const std::string& callSite)
{
    registerFunction(caller);
    registerFunction(callee);

    const std::string callerContext =
        buildFunctionName(
            caller,
            context);

    const std::string calleeContext =
        buildFunctionName(
            callee,
            context);

    graph[callerContext].push_back(calleeContext);

    CallSite site;

    site.caller = callerContext;
    site.callee = calleeContext;
    site.context = context;
    site.callSite = callSite;
    site.callString =
        context + " -> " + caller + " -> " + callee;
    site.recursive =
        isRecursiveCall(
            callee,
            context) ||
        callSite.find("recursive") != std::string::npos;

    callSites.push_back(site);
}

bool CallGraph::hasFunction(
    const std::string& functionName) const
{
    return functions.count(functionName) > 0;
}

bool CallGraph::hasEdge(
    const std::string& caller,
    const std::string& callee) const
{
    const auto found = graph.find(caller);

    if(found == graph.end())
    {
        return false;
    }

    for(const auto& target : found->second)
    {
        if(target == callee)
        {
            return true;
        }
    }

    return false;
}

bool CallGraph::hasContextSensitiveEdge(
    const std::string& caller,
    const std::string& callee,
    const std::string& context) const
{
    return hasEdge(
        buildFunctionName(
            caller,
            context),
        buildFunctionName(
            callee,
            context));
}

bool CallGraph::isRecursiveFunction(
    const std::string& functionName,
    const std::string& context) const
{
    const std::string node =
        buildFunctionName(
            functionName,
            context);

    return hasEdge(
        node,
        node);
}

bool CallGraph::hasMutualRecursion(
    const std::string& first,
    const std::string& second,
    const std::string& context) const
{
    const std::string firstNode =
        buildFunctionName(
            first,
            context);

    const std::string secondNode =
        buildFunctionName(
            second,
            context);

    return isReachableNode(
        firstNode,
        secondNode) &&
        isReachableNode(
            secondNode,
            firstNode);
}

std::vector<std::string> CallGraph::traverseFrom(
    const std::string& functionName,
    const std::string& context) const
{
    std::vector<std::string> order;
    std::vector<std::string> worklist;
    std::set<std::string> visited;

    worklist.push_back(
        buildFunctionName(
            functionName,
            context));

    while(!worklist.empty())
    {
        const std::string current =
            worklist.back();

        worklist.pop_back();

        if(visited.count(current) > 0)
        {
            continue;
        }

        visited.insert(current);
        order.push_back(current);

        const auto found =
            graph.find(current);

        if(found == graph.end())
        {
            continue;
        }

        for(auto it = found->second.rbegin();
            it != found->second.rend();
            ++it)
        {
            if(visited.count(*it) == 0)
            {
                worklist.push_back(*it);
            }
        }
    }

    return order;
}

bool CallGraph::isReachable(
    const std::string& from,
    const std::string& to,
    const std::string& context) const
{
    return isReachableNode(
        buildFunctionName(
            from,
            context),
        buildFunctionName(
            to,
            context));
}

std::string CallGraph::buildCallString(
    const std::vector<std::string>& calls) const
{
    std::string result;

    for(std::size_t i = 0; i < calls.size(); ++i)
    {
        if(i > 0)
        {
            result += " -> ";
        }

        result += calls[i];
    }

    return result;
}

void CallGraph::print() const
{
    std::cout
        << "\nContext Sensitive Call Graph:\n";

    std::cout
        << "Function"
        << "\t"
        << "Caller"
        << "\t"
        << "Callee"
        << "\t"
        << "Context"
        << "\t"
        << "Call Site"
        << "\n";

    for(const auto& site : callSites)
    {
        std::cout
            << site.callee
            << "\t"
            << site.caller
            << "\t"
            << site.callee
            << "\t"
            << site.callString
            << "\t"
            << site.callSite;

        if(site.recursive)
        {
            std::cout
                << " [recursive]";
        }

        std::cout
            << "\n";
    }
}

std::string CallGraph::buildFunctionName(
    const std::string& functionName,
    const std::string& context) const
{
    if(context.empty())
    {
        return functionName;
    }

    return functionName + "#" + context;
}

bool CallGraph::isRecursiveCall(
    const std::string& callee,
    const std::string& callString) const
{
    return callString.find(callee) != std::string::npos;
}

bool CallGraph::isReachableNode(
    const std::string& from,
    const std::string& to) const
{
    std::vector<std::string> worklist;
    std::set<std::string> visited;

    worklist.push_back(from);

    while(!worklist.empty())
    {
        const std::string current =
            worklist.back();

        worklist.pop_back();

        if(current == to)
        {
            return true;
        }

        if(visited.count(current) > 0)
        {
            continue;
        }

        visited.insert(current);

        const auto found =
            graph.find(current);

        if(found == graph.end())
        {
            continue;
        }

        for(const auto& next : found->second)
        {
            if(visited.count(next) == 0)
            {
                worklist.push_back(next);
            }
        }
    }

    return false;
}
