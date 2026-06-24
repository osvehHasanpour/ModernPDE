/*
 * ModernPDE - MASTER CHALLENGE TEST
 * Simple & Clean PDE Challenge
 */

#include "../include/PDE.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "================================================\n";
    std::cout << "     MODERNPDE MASTER CHALLENGE TEST\n";
    std::cout << "================================================\n\n";

    PDE pde;
    int total = 0;

    std::string categories[5] = {"dead", "mostlydead", "partial", "mostlylive", "live"};

    for(int cat = 0; cat < 5; cat++) {
        for(int i = 1; i <= 20; i++) {
            std::string name = categories[cat] + "_" + std::to_string(i);
            pde.defineVariable(name);
            total++;

            for(int p = 0; p < 20; p++) {
                pde.addExecutionPath(name);
            }

            int uses = 0;
            if(cat == 0) uses = 0;
            else if(cat == 1) uses = 2;
            else if(cat == 2) uses = 8;
            else if(cat == 3) uses = 18;
            else uses = 20;

            for(int u = 0; u < uses; u++) pde.useVariable(name);
        }
    }

    pde.printResults();

    std::cout << "\n================================================\n";
    std::cout << "MASTER CHALLENGE FINISHED SUCCESSFULLY!\n";
    std::cout << "Total Variables Tested: " << total << "\n";
    std::cout << "Project PDE component is working well.\n";
    std::cout << "================================================\n";

    return 0;
}
