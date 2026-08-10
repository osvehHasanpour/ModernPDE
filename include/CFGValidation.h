#pragma once

#include "CFG.h"

#include <cstddef>
#include <string>
#include <vector>

struct CFGFunctionBounds
{
    std::string name;

    int entryBlock = -1;

    int exitBlock = -1;
};

class CFGValidation
{
public:

    struct Report
    {
        bool valid = true;

        std::vector<int> unreachable;

        std::vector<int> isolated;

        std::vector<std::string> errors;

        struct FunctionReport
        {
            std::string name;

            int entryBlock = -1;

            int exitBlock = -1;

            bool validEntry = true;

            bool validExit = true;
        };

        std::vector<FunctionReport> functions;
    };

    static Report validate(
        const CFG& cfg,
        int entryBlock = 0,
        const std::vector<CFGFunctionBounds>&
            functions = {});

    static void print(
        const Report& report);
};
