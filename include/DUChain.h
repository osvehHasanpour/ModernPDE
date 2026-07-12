#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

struct Definition
{
    std::string variable;
    int line;
};

struct Use
{
    std::string variable;
    int line;
};

struct DUChain
{
    Definition def;
    Use use;
};

class DUChainAnalysis
{
public:

    void addDefinition(
        const std::string& variable,
        int line);

    void addUse(
        const std::string& variable,
        int line);

    void buildChains();

    void print();

    std::size_t chainCount() const;

private:

    std::vector<Definition> definitions;

    std::vector<Use> uses;

    std::vector<DUChain> chains;
};