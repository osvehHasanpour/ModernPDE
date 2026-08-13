#pragma once

#include "IR.h"
#include <string>
#include <unordered_map>

class FaintAnalysis
{
public:

    void run(FunctionIR& F);

private:

    std::unordered_map<
        std::string,
        bool
    > faint;
};