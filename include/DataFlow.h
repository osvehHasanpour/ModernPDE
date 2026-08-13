#pragma once

#include <set>
#include <string>

struct DataFlowSet
{
    std::set<std::string> in;

    std::set<std::string> out;
};

DataFlowSet unionSets(
    const DataFlowSet& left,
    const DataFlowSet& right);

bool equalSets(
    const DataFlowSet& left,
    const DataFlowSet& right);
