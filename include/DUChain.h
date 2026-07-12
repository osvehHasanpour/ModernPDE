#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

struct Definition
{
    std::string variable;

    int blockId = -1;

    int line = 0;

    int defId = -1;
};

struct Use
{
    std::string variable;

    int blockId = -1;

    int line = 0;
};

struct DUChain
{
    Definition def;

    Use use;
};

class CFG;

namespace ModernPDE
{
struct ASTNode;
}

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

    void buildFromCFG(
        const CFG& cfg,
        const std::unordered_map<
            int,
            std::vector<ModernPDE::ASTNode*>>&
            blockStatements);

    void print();

    std::size_t chainCount() const;

    const std::vector<DUChain>&
    getChains() const;

    const std::vector<Definition>&
    getDefinitions() const;

    const std::vector<Use>&
    getUses() const;

private:

    std::vector<Definition> definitions;

    std::vector<Use> uses;

    std::vector<DUChain> chains;
};
