#pragma once

#include "Phase8Optimizer.h"
#include "PDE.h"
#include "CFG.h"
#include "DUChain.h"
#include <string>
#include <vector>

namespace Phase8
{

/// Phase 8 benchmarking engine with Phase 7 comparison
class BenchmarkPhase8
{
public:
    /// Run comparative benchmark
    void runComparison(
        const PDE& phase7Pde,
        const CFG& phase7Cfg,
        const DUChainAnalysis& phase7Du,
        int variableCount = 1000);

    /// Run scalability benchmark
    void runScalability();

    /// Generate benchmark report
    std::string generateReport() const;

    /// Save report to file
    void saveReport(const std::string& filename) const;

    /// Get metrics
    const BenchmarkMetrics& getPhase8Metrics() const { return phase8Metrics; }
    const BenchmarkMetrics& getPhase7Metrics() const { return phase7Metrics; }

private:
    BenchmarkMetrics phase8Metrics;
    BenchmarkMetrics phase7Metrics;
    std::vector<std::string> reportLines;

    void addReportLine(const std::string& line);
    void runBenchmarkVariant(int variableCount, bool useOptimizations);
    void printComparison();
    std::string formatMetrics(const BenchmarkMetrics& metrics) const;
};

} // namespace Phase8
