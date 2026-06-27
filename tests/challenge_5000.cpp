/*
 * ModernPDE - CHALLENGE TEST (5000 Variables)
 * All 5 categories, real metrics computed via Metrics API.
 */

#include "../include/PDE.h"
#include "../include/Metrics.h"
#include <iostream>
#include <iomanip>
#include <string>

struct Category
{
    std::string prefix;
    std::string expected;
    int         count;
    int         paths;
    int         uses;
};

static bool isPositive(const std::string& label)
{
    return label == "DEAD" || label == "PARTIALLY DEAD";
}

int main()
{
    std::cout << "================================================\n";
    std::cout << "  MODERNPDE CHALLENGE TEST  —  5000 Variables\n";
    std::cout << "================================================\n\n";

    // 5000 / 5 = 1000 variables per category
    // paths = 40
    const Category categories[] = {
        { "dead",       "DEAD",           1000, 40,  0  },  // 0/40  = 0.00  → DEAD
        { "mostlydead", "MOSTLY DEAD",    1000, 40,  4  },  // 4/40  = 0.10  → MOSTLY DEAD
        { "partial",    "PARTIALLY DEAD", 1000, 40, 16  },  // 16/40 = 0.40  → PARTIALLY DEAD
        { "mostlylive", "MOSTLY LIVE",    1000, 40, 36  },  // 36/40 = 0.90  → MOSTLY LIVE
        { "live",       "LIVE",           1000, 40, 40  },  // 40/40 = 1.00  → LIVE
    };

    PDE pde;

    std::cout << "Populating 5000 variables (200,000 total execution paths)...\n\n";

    for(const auto& cat : categories)
    {
        for(int i = 1; i <= cat.count; i++)
        {
            std::string name = cat.prefix + "_" + std::to_string(i);
            pde.defineVariable(name);
            for(int p = 0; p < cat.paths; p++) pde.addExecutionPath(name);
            for(int u = 0; u < cat.uses;  u++) pde.useVariable(name);
        }
    }

    // ---- verify ----
    int correct = 0, total = 0;
    int tp = 0, fp = 0, fn = 0;

    for(const auto& cat : categories)
    {
        for(int i = 1; i <= cat.count; i++)
        {
            std::string name   = cat.prefix + "_" + std::to_string(i);
            std::string actual = pde.classify(name);
            bool match         = (actual == cat.expected);

            total++;
            if(match) correct++;

            bool expPos = isPositive(cat.expected);
            bool actPos = isPositive(actual);
            if(expPos && actPos)  tp++;
            if(!expPos && actPos) fp++;
            if(expPos && !actPos) fn++;
        }
    }

    // ---- per-category breakdown ----
    std::cout << "Per-Category Results:\n";
    std::cout << std::left
              << std::setw(18) << "Category"
              << std::setw(18) << "Expected"
              << std::setw(8)  << "Count"
              << std::setw(8)  << "Ratio"
              << "Status\n";
    std::cout << std::string(62, '-') << "\n";

    for(const auto& cat : categories)
    {
        std::string sample = cat.prefix + "_1";
        std::string actual = pde.classify(sample);
        double      ratio  = pde.getUsageRatio(sample);
        bool        pass   = (actual == cat.expected);

        std::cout << std::left
                  << std::setw(18) << cat.prefix
                  << std::setw(18) << cat.expected
                  << std::setw(8)  << cat.count
                  << std::fixed << std::setprecision(2)
                  << std::setw(8)  << ratio
                  << (pass ? "PASS" : "FAIL  got: " + actual)
                  << "\n";
    }

    // ---- metrics ----
    std::cout << "\n------------------------------------------------\n";
    std::cout << "Computed Metrics (5000 variables):\n\n";

    double acc  = Metrics::accuracy(correct, total);
    double prec = Metrics::precision(tp, fp);
    double rec  = Metrics::recall(tp, fn);
    double f1   = Metrics::f1Score(tp, fp, fn);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Correct      : " << correct << " / " << total << "\n";
    std::cout << "  TP / FP / FN : " << tp << " / " << fp << " / " << fn << "\n\n";
    std::cout << "  Accuracy     : " << acc  << "\n";
    std::cout << "  Precision    : " << prec << "\n";
    std::cout << "  Recall       : " << rec  << "\n";
    std::cout << "  F1 Score     : " << f1   << "\n";

    bool passed = (correct == total);
    std::cout << "\n================================================\n";
    if(passed)
    {
        std::cout << "RESULT : PASS\n";
        std::cout << "ModernPDE classified all 5000 variables correctly.\n";
    }
    else
    {
        std::cout << "RESULT : FAIL\n";
        std::cout << (total - correct) << " variable(s) misclassified.\n";
    }
    std::cout << "================================================\n";

    return passed ? 0 : 1;
}
