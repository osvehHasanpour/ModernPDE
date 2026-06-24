#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

struct CallSite
{
    std::string caller;

    std::string callee;

    std::string context;

    std::string callSite;

    std::string callString;

    bool recursive = false;
};

class CallGraph
{
public:

    void registerFunction(
        const std::string& functionName);

    void addEdge(
        const std::string& caller,
        const std::string& callee);

    void addContextSensitiveEdge(
        const std::string& caller,
        const std::string& callee,
        const std::string& context,
        const std::string& callSite);

    bool hasFunction(
        const std::string& functionName) const;

    bool hasEdge(
        const std::string& caller,
        const std::string& callee) const;

    bool hasContextSensitiveEdge(
        const std::string& caller,
        const std::string& callee,
        const std::string& context) const;

    bool isRecursiveFunction(
        const std::string& functionName,
        const std::string& context = "") const;

    bool hasMutualRecursion(
        const std::string& first,
        const std::string& second,
        const std::string& context = "") const;

    std::vector<std::string> traverseFrom(
        const std::string& functionName,
        const std::string& context = "") const;

    bool isReachable(
        const std::string& from,
        const std::string& to,
        const std::string& context = "") const;

    std::string buildCallString(
        const std::vector<std::string>& calls);

    void print();

private:

    std::string buildFunctionName(
        const std::string& functionName,
        const std::string& context) const;

    bool isRecursiveCall(
        const std::string& callee,
        const std::string& callString) const;

    bool isReachableNode(
        const std::string& from,
        const std::string& to) const;

private:

    std::set<
        std::string
    > functions;

    std::map<
        std::string,
        std::vector<std::string>
    > graph;

    std::vector<
        CallSite
    > callSites;
};
