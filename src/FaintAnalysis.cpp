#include "FaintAnalysis.h"

#include <iostream>

void FaintAnalysis::run(FunctionIR& F)
{
    faint.clear();

    bool changed = true;

    while(changed)
    {
        changed = false;

        for(auto it =
            F.instructions.rbegin();
            it != F.instructions.rend();
            ++it)
        {
            auto& inst = *it;

            bool allFaint = true;

            for(auto& u : inst.uses)
            {
                if(!faint[u])
                {
                    allFaint = false;
                    break;
                }
            }

            if(allFaint)
            {
                inst.partialDead = true;
            }
            else
            {
                faint[inst.result] = false;
            }
        }
    }

    std::cout
        << "Faint Analysis Finished\n";
}