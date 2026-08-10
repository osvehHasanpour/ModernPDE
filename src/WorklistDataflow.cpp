#include "WorklistDataflow.h"
#include "Phase8Optimizer.h"
#include <iostream>

namespace Phase8
{

void WorklistDataflow::initialize(int entryBlock)
{
    if(cfg.hasBlock(entryBlock))
    {
        addToWorklist(entryBlock, 100);
    }
}

void WorklistDataflow::addToWorklist(int blockId, int priority)
{
    if(onWorklist.count(blockId) == 0)
    {
        WorklistEntry entry;
        entry.blockId = blockId;
        entry.priority = priority;
        worklist.push(entry);
        onWorklist.insert(blockId);
    }
}

bool WorklistDataflow::processNext()
{
    if(worklist.empty())
        return false;

    WorklistEntry entry = worklist.top();
    worklist.pop();
    onWorklist.erase(entry.blockId);

    stats.itemsProcessed++;
    stats.iterations++;

    return true;
}

bool WorklistDataflow::isWorklistEmpty() const
{
    return worklist.empty();
}

void WorklistDataflow::saturate()
{
    while(!worklist.empty())
    {
        processNext();
    }
    stats.maxWorklistSize = std::max(stats.maxWorklistSize, (uint64_t)worklist.size());
}

} // namespace Phase8
