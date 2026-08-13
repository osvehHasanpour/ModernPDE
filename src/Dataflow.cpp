#include "DataFlow.h"

#include <algorithm>
#include <iterator>

DataFlowSet unionSets(
    const DataFlowSet& left,
    const DataFlowSet& right)
{
    DataFlowSet result;

    std::set_union(
        left.in.begin(),
        left.in.end(),
        right.in.begin(),
        right.in.end(),
        std::inserter(result.in, result.in.end()));

    std::set_union(
        left.out.begin(),
        left.out.end(),
        right.out.begin(),
        right.out.end(),
        std::inserter(result.out, result.out.end()));

    return result;
}

bool equalSets(
    const DataFlowSet& left,
    const DataFlowSet& right)
{
    return left.in == right.in &&
           left.out == right.out;
}
