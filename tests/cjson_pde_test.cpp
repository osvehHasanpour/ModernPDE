/*
 * ModernPDE x cJSON Integration Test
 *
 * Reads test cases from pde_cases.json using cJSON,
 * feeds each case into PDE, verifies the classification,
 * and reports real Metrics.
 *
 * Build (from ModernPDE root):
 *   gcc  -O2 -c ../cJSON-master/cJSON.c -o cJSON.o
 *   g++  -std=c++17 -O2 \
 *        -I include -I ../cJSON-master \
 *        tests/cjson_pde_test.cpp \
 *        src/PDE.cpp src/Metrics.cpp \
 *        cJSON.o -lm -o cjson_pde_test
 *   ./cjson_pde_test tests/pde_cases.json
 */

#include "PDE.h"
#include "Metrics.h"

extern "C" {
#include "cJSON.h"
}

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

// ---------------------------------------------------------------------------
// Simple pass/fail counters
// ---------------------------------------------------------------------------
static int g_total  = 0;
static int g_passes = 0;
static int g_fails  = 0;

#define CHECK(cond, msg) do { \
    g_total++; \
    if(cond) { g_passes++; } \
    else { \
        g_fails++; \
        std::cout << "  FAIL  " << (msg) << "\n"; \
    } \
} while(0)

// ---------------------------------------------------------------------------
// Read entire file into string
// ---------------------------------------------------------------------------
static std::string read_file(const std::string& path)
{
    std::ifstream f(path);
    if(!f.is_open())
    {
        std::cerr << "ERROR: cannot open file: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// isPositive: "positive class" for precision/recall calculation
// ---------------------------------------------------------------------------
static bool isPositive(const std::string& label)
{
    return label == "DEAD" || label == "PARTIALLY DEAD";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::string json_path = "tests/pde_cases.json";
    if(argc >= 2) json_path = argv[1];

    std::cout << "================================================\n";
    std::cout << "  ModernPDE x cJSON Integration Test\n";
    std::cout << "  Loading: " << json_path << "\n";
    std::cout << "================================================\n\n";

    // ---- Step 1: read JSON file ----
    std::string json_text = read_file(json_path);
    if(json_text.empty())
    {
        std::cerr << "FATAL: empty or missing JSON file.\n";
        return 1;
    }

    // ---- Step 2: parse with cJSON ----
    cJSON* root = cJSON_Parse(json_text.c_str());
    if(!root)
    {
        const char* err = cJSON_GetErrorPtr();
        std::cerr << "FATAL: cJSON parse error near: "
                  << (err ? err : "unknown") << "\n";
        return 1;
    }

    // ---- Step 3: read metadata ----
    cJSON* suite_name = cJSON_GetObjectItem(root, "test_suite");
    cJSON* version    = cJSON_GetObjectItem(root, "version");
    if(cJSON_IsString(suite_name))
        std::cout << "Suite   : " << suite_name->valuestring << "\n";
    if(cJSON_IsString(version))
        std::cout << "Version : " << version->valuestring << "\n";

    // ---- Step 4: iterate cases ----
    cJSON* cases = cJSON_GetObjectItem(root, "cases");
    if(!cJSON_IsArray(cases))
    {
        std::cerr << "FATAL: 'cases' is not an array in JSON.\n";
        cJSON_Delete(root);
        return 1;
    }

    int case_count = cJSON_GetArraySize(cases);
    std::cout << "Cases   : " << case_count << "\n\n";

    // Metrics counters
    int correct = 0, total_vars = 0;
    int tp = 0, fp = 0, fn = 0;

    // Column header
    std::cout << std::left
              << std::setw(18) << "Name"
              << std::setw(7)  << "Paths"
              << std::setw(7)  << "Uses"
              << std::setw(7)  << "Ratio"
              << std::setw(18) << "Expected"
              << std::setw(18) << "Got"
              << "Result\n";
    std::cout << std::string(78, '-') << "\n";

    PDE pde;

    cJSON* cas = NULL;
    cJSON_ArrayForEach(cas, cases)
    {
        // Read fields from JSON
        cJSON* j_name     = cJSON_GetObjectItem(cas, "name");
        cJSON* j_paths    = cJSON_GetObjectItem(cas, "paths");
        cJSON* j_uses     = cJSON_GetObjectItem(cas, "uses");
        cJSON* j_expected = cJSON_GetObjectItem(cas, "expected");

        if(!cJSON_IsString(j_name)   ||
           !cJSON_IsNumber(j_paths)  ||
           !cJSON_IsNumber(j_uses)   ||
           !cJSON_IsString(j_expected))
        {
            std::cout << "  SKIP  (malformed case in JSON)\n";
            continue;
        }

        std::string name     = j_name->valuestring;
        int         paths    = (int)j_paths->valuedouble;
        int         uses     = (int)j_uses->valuedouble;
        std::string expected = j_expected->valuestring;

        // Feed into PDE
        pde.defineVariable(name);
        for(int p = 0; p < paths; p++) pde.addExecutionPath(name);
        for(int u = 0; u < uses;  u++) pde.useVariable(name);

        // Classify
        std::string actual = pde.classify(name);
        double ratio = pde.getUsageRatio(name);
        bool match = (actual == expected);

        // Count
        total_vars++;
        if(match) correct++;

        bool expPos = isPositive(expected);
        bool actPos = isPositive(actual);
        if(expPos && actPos)  tp++;
        if(!expPos && actPos) fp++;
        if(expPos && !actPos) fn++;

        // Print row
        std::cout << std::left
                  << std::setw(18) << name
                  << std::setw(7)  << paths
                  << std::setw(7)  << uses
                  << std::fixed << std::setprecision(2)
                  << std::setw(7)  << ratio
                  << std::setw(18) << expected
                  << std::setw(18) << actual
                  << (match ? "PASS" : "FAIL")
                  << "\n";

        CHECK(match, "name=" + name
                   + " expected=" + expected
                   + " got=" + actual);
    }

    cJSON_Delete(root);

    // ---- Step 5: real metrics ----
    std::cout << "\n" << std::string(78, '-') << "\n";
    std::cout << "Computed Metrics (" << total_vars << " variables):\n\n";

    double acc  = Metrics::accuracy(correct, total_vars);
    double prec = Metrics::precision(tp, fp);
    double rec  = Metrics::recall(tp, fn);
    double f1   = Metrics::f1Score(tp, fp, fn);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Correct      : " << correct << " / " << total_vars << "\n";
    std::cout << "  TP / FP / FN : " << tp << " / " << fp << " / " << fn << "\n\n";
    std::cout << "  Accuracy     : " << acc  << "\n";
    std::cout << "  Precision    : " << prec << "\n";
    std::cout << "  Recall       : " << rec  << "\n";
    std::cout << "  F1 Score     : " << f1   << "\n";

    // ---- Step 6: final verdict ----
    bool passed = (g_fails == 0);
    std::cout << "\n================================================\n";
    std::cout << "  Total : " << g_total
              << "  Pass  : " << g_passes
              << "  Fail  : " << g_fails << "\n";
    if(passed)
        std::cout << "  RESULT : ALL TESTS PASSED\n";
    else
        std::cout << "  RESULT : " << g_fails << " TEST(S) FAILED\n";
    std::cout << "================================================\n";

    return passed ? 0 : 1;
}
