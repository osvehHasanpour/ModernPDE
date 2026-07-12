#include "DUChain.h"

#include <iostream>

void DUChainAnalysis::addDefinition(
    const std::string& variable,
    int line)
{
    definitions.push_back(
    {
        variable,
        line
    });
}

void DUChainAnalysis::addUse(
    const std::string& variable,
    int line)
{
    uses.push_back(
    {
        variable,
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
        << ", line "
        << chain.def.line
        << ") -> Use(line "
        << chain.use.line
        << ")\n";
    }
}

std::size_t DUChainAnalysis::chainCount() const
{
    return chains.size();
}