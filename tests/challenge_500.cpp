/*
 * ModernPDE - CHALLENGE TEST (500 Variables)
 * All 5 categories, real metrics computed via Metrics API.
 */

#include "../include/PDE.h"
#include "../include/Metrics.h"
#include <iostream>
#include <iomanip>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct Category
{
    std::string prefix;
    std::string expected;
    int         count;
    int         paths;
    int         uses;
};

struct Results
{
    int correct = 0;
    int tp      = 0;   // classified dead/partial when expected dead/partial
    int fp      = 0;   // classified dead/partial when should not be
    int fn      = 0;   // missed dead/partial when should be
};

// "positive" class = DEAD or PARTIALLY DEAD  (the interesting analysis targets)
static bool isPositive(const std::string& label)
{
    return label == "DEAD" || label == "PARTIALLY DEAD";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "================================================\n";
    std::cout << "   MODERNPDE CHALLENGE TEST  —  500 Variables\n";
    std::cout << "================================================\n\n";

    // 500 / 5 = 100 variables per category
    // paths = 20, chosen so every use-ratio hits a distinct bucket cleanly
    const Category categories[] = {
        { "dead",       "DEAD",           100, 20,  0  },  // 0/20  = 0.00  → DEAD
        { "mostlydead", "MOSTLY DEAD",    100, 20,  2  },  // 2/20  = 0.10  → MOSTLY DEAD
        { "partial",    "PARTIALLY DEAD", 100, 20,  8  },  // 8/20  = 0.40  → PARTIALLY DEAD
        { "mostlylive", "MOSTLY LIVE",    100, 20, 18  },  // 18/20 = 0.90  → MOSTLY LIVE
        { "live",       "LIVE",           100, 20, 20  },  // 20/20 = 1.00  → LIVE
    };

    PDE pde;

    // ---- populate ----
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

    // ---- verify every variable against its expected label ----
    Results res;
    int total = 0;

    for(const auto& cat : categories)
    {
        for(int i = 1; i <= cat.count; i++)
        {
            std::string name   = cat.prefix + "_" + std::to_string(i);
            std::string actual = pde.classify(name);
            bool match         = (actual == cat.expected);

            total++;
            if(match) res.correct++;

            bool expPos = isPositive(cat.expected);
            bool actPos = isPositive(actual);

            if(expPos && actPos)  res.tp++;
            if(!expPos && actPos) res.fp++;
            if(expPos && !actPos) res.fn++;
        }
    }

    // ---- per-category breakdown ----
    std::cout << "Per-Category Results:\n";
    std::cout << std::left
              << std::setw(18) << "Category"
              << std::setw(18) << "Expected"
              << std::setw(6)  << "Count"
              << std::setw(8)  << "Ratio"
              << "Status\n";
    std::cout << std::string(60, '-') << "\n";

    for(const auto& cat : categories)
    {
        // spot-check first variable of each group
        std::string sample  = cat.prefix + "_1";
        std::string actual  = pde.classify(sample);
        double      ratio   = pde.getUsageRatio(sample);
        bool        pass    = (actual == cat.expected);

        std::cout << std::left
                  << std::setw(18) << cat.prefix
                  << std::setw(18) << cat.expected
                  << std::setw(6)  << cat.count
                  << std::fixed << std::setprecision(2)
                  << std::setw(8)  << ratio
                  << (pass ? "PASS" : "FAIL  got: " + actual)
                  << "\n";
    }

    // ---- metrics ----
    std::cout << "\n------------------------------------------------\n";
    std::cout << "Computed Metrics (500 variables):\n\n";

    double acc  = Metrics::accuracy(res.correct, total);
    double prec = Metrics::precision(res.tp, res.fp);
    double rec  = Metrics::recall(res.tp, res.fn);
    double f1   = Metrics::f1Score(res.tp, res.fp, res.fn);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Correct    : " << res.correct << " / " << total   << "\n";
    std::cout << "  TP / FP / FN : " << res.tp << " / " << res.fp << " / " << res.fn << "\n\n";
    std::cout << "  Accuracy   : " << acc  << "\n";
    std::cout << "  Precision  : " << prec << "\n";
    std::cout << "  Recall     : " << rec  << "\n";
    std::cout << "  F1 Score   : " << f1   << "\n";

    // ---- pass / fail ----
    bool passed = (res.correct == total);
    std::cout << "\n================================================\n";
    if(passed)
    {
        std::cout << "RESULT : PASS\n";
        std::cout << "ModernPDE classified all 500 variables correctly.\n";
    }
    else
    {
        std::cout << "RESULT : FAIL\n";
        std::cout << (total - res.correct) << " variable(s) misclassified.\n";
    }
    std::cout << "================================================\n";

    return passed ? 0 : 1;
}
