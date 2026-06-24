#pragma once

#include <set>
#include <string>

struct DataFlowSet
{
    std::set<std::string> in;

    std::set<std::string> out;
};