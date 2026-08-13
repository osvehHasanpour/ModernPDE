#include "DUChain.h"
#include "CFG.h"
#include "Parser.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace
{

bool isIdentifierChar(
    char c)
{
    return std::isalnum(
               static_cast<unsigned char>(c)) ||
           c == '_';
}

bool isKeyword(
    const std::string& word)
{
    static const std::unordered_set<std::string>
        keywords =
    {
        "if", "else", "for", "while", "do",
        "switch", "case", "default",
        "break", "continue",
        "return", "int", "void", "float",
        "double", "char", "bool", "long",
        "short", "signed", "unsigned", "auto",
        "true", "false", "nullptr", "rand"
    };

    return keywords.count(word) != 0;
}

void extractIdentifiers(
    const std::string& text,
    std::vector<std::string>& out)
{
    std::size_t i = 0;

    while(i < text.size())
    {
        if(!isIdentifierChar(text[i]) ||
           std::isdigit(
               static_cast<unsigned char>(
                   text[i])))
        {
            i++;
            continue;
        }

        std::size_t start = i;

        while(i < text.size() &&
              isIdentifierChar(text[i]))
        {
            i++;
        }

        std::string id =
            text.substr(start, i - start);

        if(!isKeyword(id))
        {
            out.push_back(id);
        }
    }
}

void collectDefsAndUses(
    ModernPDE::ASTNode* node,
    int blockId,
    std::vector<Definition>& defs,
    std::vector<Use>& uses,
    int& nextDefId)
{
    if(node == nullptr)
    {
        return;
    }

    if(node->type ==
       ModernPDE::ASTNodeType::Variable)
    {
        Definition def;
        def.variable = node->name;
        def.blockId  = blockId;
        def.line     = node->line;
        def.defId    = nextDefId++;
        defs.push_back(def);

        if(node->child(0) != nullptr)
        {
            std::vector<std::string> ids;
            extractIdentifiers(
                node->child(0)->name,
                ids);

            for(const auto& id : ids)
            {
                Use use;
                use.variable = id;
                use.blockId  = blockId;
                use.line     = node->line;
                uses.push_back(use);
            }
        }

        return;
    }

    if(node->type ==
           ModernPDE::ASTNodeType::Expression ||
       node->type ==
           ModernPDE::ASTNodeType::ReturnStatement)
    {
        std::vector<std::string> ids;
        extractIdentifiers(
            node->name,
            ids);

        for(const auto& id : ids)
        {
            Use use;
            use.variable = id;
            use.blockId  = blockId;
            use.line     = node->line;
            uses.push_back(use);
        }
    }
}

}

void DUChainAnalysis::addDefinition(
    const std::string& variable,
    int line)
{
    definitions.push_back(
    {
        variable,
        -1,
        line,
        -1
    });
}

void DUChainAnalysis::addUse(
    const std::string& variable,
    int line)
{
    uses.push_back(
    {
        variable,
        -1,
        line
    });
}

void DUChainAnalysis::buildChains()
{
    chains.clear();

    for(const auto& use : uses)
    {
        const Definition* latestDef =
            nullptr;

        for(const auto& def : definitions)
        {
            if(def.variable != use.variable)
            {
                continue;
            }

            if(def.line < use.line)
            {
                if(latestDef == nullptr ||
                   def.line > latestDef->line)
                {
                    latestDef = &def;
                }
            }
        }

        if(latestDef != nullptr)
        {
            chains.push_back(
            {
                *latestDef,
                use
            });
        }
    }
}

void DUChainAnalysis::buildFromCFG(
    const CFG& cfg,
    const std::unordered_map<
        int,
        std::vector<ModernPDE::ASTNode*>>&
        blockStatements)
{
    definitions.clear();
    uses.clear();
    chains.clear();

    int nextDefId = 0;

    for(const auto& entry : blockStatements)
    {
        for(ModernPDE::ASTNode* node :
            entry.second)
        {
            collectDefsAndUses(
                node,
                entry.first,
                definitions,
                uses,
                nextDefId);
        }
    }

    if(definitions.empty())
    {
        return;
    }

    std::unordered_map<
        int,
        std::unordered_set<int>> gen;

    std::unordered_map<
        int,
        std::unordered_set<int>> kill;

    std::unordered_map<
        std::string,
        std::vector<int>> defsByVar;

    for(const auto& def : definitions)
    {
        defsByVar[def.variable].push_back(
            def.defId);
    }

    for(const auto& def : definitions)
    {
        gen[def.blockId].insert(def.defId);

        for(int otherId :
            defsByVar[def.variable])
        {
            if(otherId != def.defId)
            {
                kill[def.blockId].insert(
                    otherId);
            }
        }
    }

    std::unordered_map<
        int,
        std::unordered_set<int>> inSet;
    std::unordered_map<
        int,
        std::unordered_set<int>> outSet;

    for(const auto& block : cfg.getBlocks())
    {
        inSet[block.id];
        outSet[block.id];
    }

    bool changed = true;

    while(changed)
    {
        changed = false;

        for(const auto& block : cfg.getBlocks())
        {
            std::unordered_set<int> newIn;

            for(int pred : block.preds)
            {
                const auto& predOut =
                    outSet[pred];

                newIn.insert(
                    predOut.begin(),
                    predOut.end());
            }

            if(newIn != inSet[block.id])
            {
                inSet[block.id] = newIn;
                changed = true;
            }

            std::unordered_set<int> newOut =
                gen[block.id];

            for(int defId : inSet[block.id])
            {
                if(kill[block.id].count(
                       defId) == 0)
                {
                    newOut.insert(defId);
                }
            }

            if(newOut != outSet[block.id])
            {
                outSet[block.id] = newOut;
                changed = true;
            }
        }
    }

    std::unordered_map<
        int,
        Definition> defById;

    for(const auto& def : definitions)
    {
        defById[def.defId] = def;
    }

    for(const auto& use : uses)
    {
        const auto& reaching =
            inSet[use.blockId];

        for(int defId : reaching)
        {
            const Definition& def =
                defById[defId];

            if(def.variable != use.variable)
            {
                continue;
            }

            chains.push_back(
            {
                def,
                use
            });
        }
    }
}

void DUChainAnalysis::print()
{
    std::cout
    << "\n====================\n";

    std::cout
    << "DU CHAINS\n";

    std::cout
    << "====================\n";

    for(const auto& chain : chains)
    {
        std::cout
        << "Def("
        << chain.def.variable
        << ", block "
        << chain.def.blockId
        << ", line "
        << chain.def.line
        << ") -> Use(block "
        << chain.use.blockId
        << ", line "
        << chain.use.line
        << ")\n";
    }
}

std::size_t DUChainAnalysis::chainCount() const
{
    return chains.size();
}

const std::vector<DUChain>&
DUChainAnalysis::getChains() const
{
    return chains;
}

const std::vector<Definition>&
DUChainAnalysis::getDefinitions() const
{
    return definitions;
}

const std::vector<Use>&
DUChainAnalysis::getUses() const
{
    return uses;
}
