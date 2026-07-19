#pragma once

#include "CFG.h"
#include "CFGValidation.h"

#include <cstddef>
#include <string>
#include <vector>

class PathEnumeration;

class CFGStatistics
{
public:

    struct Stats
    {
        std::size_t blocks = 0;

        std::size_t edges = 0;

        int cyclomaticComplexity = 0;

        int maxDepth = 0;

        double avgSuccessors = 0.0;

        double avgBlockSize = 0.0;

        std::size_t executionPaths = 0;
    };

    struct FunctionStats : Stats
    {
        std::string functionName;

        int entryBlock = -1;

        int exitBlock = -1;
    };

    static Stats compute(
        const CFG& cfg,
        int entryBlock = 0,
        std::size_t pathCount = 0);

    static std::vector<FunctionStats>
    computePerFunction(
        const CFG& cfg,
        const std::vector<CFGFunctionBounds>&
            functions,
        const PathEnumeration& paths);

    static void print(
        const Stats& stats);

    static void printPerFunction(
        const std::vector<FunctionStats>& stats);
};
