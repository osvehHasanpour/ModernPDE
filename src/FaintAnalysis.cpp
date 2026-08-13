#include "FaintAnalysis.h"
#include "Statistics.h"

#include <iostream>

namespace
{

bool hasSideEffect(
    InstType type)
{
    return type == InstType::Call ||
           type == InstType::Branch ||
           type == InstType::Return ||
           type == InstType::FieldStore;
}

}

void FaintAnalysis::run(FunctionIR& F)
{
    faint.clear();

    for(const auto& inst : F.instructions)
    {
        if(!inst.result.empty())
        {
            faint[inst.result] = true;
        }

        for(const auto& use : inst.uses)
        {
            if(faint.find(use) == faint.end())
            {
                faint[use] = true;
            }
        }
    }

    bool changed = true;

    while(changed)
    {
        changed = false;

        for(auto it = F.instructions.rbegin();
            it != F.instructions.rend();
            ++it)
        {
            auto& inst = *it;

            const bool resultFaint =
                inst.result.empty() ||
                faint[inst.result];

            if(hasSideEffect(inst.type) || !resultFaint)
            {
                if(inst.partialDead)
                {
                    inst.partialDead = false;
                    changed = true;
                }

                for(const auto& use : inst.uses)
                {
                    auto found = faint.find(use);

                    if(found != faint.end() && found->second)
                    {
                        found->second = false;
                        changed = true;
                    }
                }
            }
            else if(!inst.partialDead)
            {
                inst.partialDead = true;
                changed = true;
            }
        }
    }

    Statistics stats;
    stats.add("instructions", F.instructions.size());

    for(const auto& inst : F.instructions)
    {
        if(inst.partialDead)
        {
            stats.add("faint");
        }
    }

    stats.print("Faint analysis");

    std::cout
        << "Faint Analysis Finished\n";
}
