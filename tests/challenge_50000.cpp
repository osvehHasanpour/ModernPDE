/*
 * ModernPDE - CHALLENGE TEST (50000 Variables)
 * All 5 categories, real metrics computed via Metrics API.
 * Also measures wall-clock time to expose any scaling issues.
 */

#include "../include/PDE.h"
#include "../include/Metrics.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

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
    std::cout << " MODERNPDE CHALLENGE TEST  —  50,000 Variables\n";
    std::cout << "================================================\n\n";

    // 50000 / 5 = 10000 variables per category
    // paths = 50, use-ratios kept identical to the 500 / 5000 tests
    // so results are directly comparable across scales
    const Category categories[] = {
        { "dead",       "DEAD",            10000, 50,  0  },  // 0/50  = 0.00 → DEAD
        { "mostlydead", "MOSTLY DEAD",     10000, 50,  5  },  // 5/50  = 0.10 → MOSTLY DEAD
        { "partial",    "PARTIALLY DEAD",  10000, 50, 20  },  // 20/50 = 0.40 → PARTIALLY DEAD
        { "mostlylive", "MOSTLY LIVE",     10000, 50, 45  },  // 45/50 = 0.90 → MOSTLY LIVE
        { "live",       "LIVE",            10000, 50, 50  },  // 50/50 = 1.00 → LIVE
    };

    const int totalPaths = 50000 * 50;   // 2,500,000

    std::cout << "Scale:\n";
    std::cout << "  Variables        : 50,000\n";
    std::cout << "  Paths / variable : 50\n";
    std::cout << "  Total paths      : 2,500,000\n\n";

    // ---- populate & time it ----
    auto t0 = std::chrono::steady_clock::now();

    PDE pde;

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

    auto t1 = std::chrono::steady_clock::now();
    double buildMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "Build time : " << std::fixed << std::setprecision(1) << buildMs << " ms\n\n";

    // ---- verify & time classification ----
    auto t2 = std::chrono::steady_clock::now();

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

    auto t3 = std::chrono::steady_clock::now();
    double classifyMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    double totalMs    = std::chrono::duration<double, std::milli>(t3 - t0).count();

    std::cout << "Classify time : " << std::fixed << std::setprecision(1) << classifyMs << " ms\n";
    std::cout << "Total time    : " << totalMs    << " ms\n\n";

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
    std::cout << "Computed Metrics (50,000 variables):\n\n";

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

    // ---- throughput ----
    if(totalMs > 0.0)
    {
        double varsPerSec = (total / totalMs) * 1000.0;
        std::cout << "\n  Throughput   : "
                  << std::fixed << std::setprecision(0)
                  << varsPerSec << " variables/sec\n";
    }

    bool passed = (correct == total);
    std::cout << "\n================================================\n";
    if(passed)
    {
        std::cout << "RESULT : PASS\n";
        std::cout << "ModernPDE classified all 50,000 variables correctly.\n";
    }
    else
    {
        std::cout << "RESULT : FAIL\n";
        std::cout << (total - correct) << " variable(s) misclassified.\n";
    }
    std::cout << "================================================\n";

    return passed ? 0 : 1;
}
