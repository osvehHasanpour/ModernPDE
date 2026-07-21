#include "DUAnalysisCache.h"
#include "Phase8Optimizer.h"
#include <algorithm>

namespace Phase8
{

void DUAnalysisCache::initialize(const DUChainAnalysis& duAnalysis, const CFG& cfg)
{
    this->cfg = &cfg;
    this->duChains = duAnalysis.getChains();
    this->definitions = duAnalysis.getDefinitions();
    this->uses = duAnalysis.getUses();

    stats.chainsAnalyzed = duChains.size();
    stats.reachingDefsComputed = definitions.size();

    computeReachingDefinitions();
}

const std::unordered_set<int>& DUAnalysisCache::getReachingDefinitions(int blockId)
{
    auto it = blockReachingDefs.find(blockId);
    if(it != blockReachingDefs.end() && it->second.isValid)
    {
        stats.cacheHits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second.outSet;
    }

    stats.cacheMisses++;
    BenchmarkState::instance().recordCacheMiss();

    static const std::unordered_set<int> empty;
    return empty;
}

const std::vector<int>& DUAnalysisCache::getReachingDefsForUse(int useId)
{
    auto it = useToReachingDefs.find(useId);
    if(it != useToReachingDefs.end())
    {
        stats.cacheHits++;
        BenchmarkState::instance().recordCacheHit();
        return it->second;
    }

    stats.cacheMisses++;
    BenchmarkState::instance().recordCacheMiss();

    static const std::vector<int> empty;
    return empty;
}

bool DUAnalysisCache::definitionReaches(int defId, int useId)
{
    if(defId < 0 || useId < 0 || useId >= static_cast<int>(uses.size()))
        return false;

    const auto& reachingDefs = getReachingDefsForUse(useId);
    return std::find(reachingDefs.begin(), reachingDefs.end(), defId) != reachingDefs.end();
}

void DUAnalysisCache::updateChangedBlocks(const std::unordered_set<int>& changedBlockIds)
{
    invalidatedBlocks = changedBlockIds;
    stats.incrementalUpdates++;
    propagateChanges(changedBlockIds);
}

void DUAnalysisCache::computeReachingDefinitions()
{
    if(!cfg)
        return;

    // Build gen and kill sets for each block
    std::unordered_map<int, std::unordered_set<int>> gen;
    std::unordered_map<int, std::unordered_set<int>> kill;
    std::unordered_map<std::string, std::vector<int>> defsByVar;

    for(const auto& def : definitions)
    {
        defsByVar[def.variable].push_back(def.defId);
    }

    for(const auto& def : definitions)
    {
        gen[def.blockId].insert(def.defId);
        for(int otherId : defsByVar[def.variable])
        {
            if(otherId != def.defId)
            {
                kill[def.blockId].insert(otherId);
            }
        }
    }

    // Worklist-based dataflow
    bool changed = true;
    int iterations = 0;

    while(changed && iterations < 1000)
    {
        changed = false;
        iterations++;

        for(const auto& block : cfg->getBlocks())
        {
            ReachingDefInfo& info = blockReachingDefs[block.id];
            info.blockId = block.id;
            info.genSet = gen[block.id];
            info.killSet = kill[block.id];

            std::unordered_set<int> newIn;
            for(int pred : block.preds)
            {
                const auto& predOut = blockReachingDefs[pred].outSet;
                newIn.insert(predOut.begin(), predOut.end());
            }

            if(newIn != info.inSet)
            {
                info.inSet = newIn;
                changed = true;
            }

            std::unordered_set<int> newOut = gen[block.id];
            for(int defId : info.inSet)
            {
                if(kill[block.id].count(defId) == 0)
                {
                    newOut.insert(defId);
                }
            }

            if(newOut != info.outSet)
            {
                info.outSet = newOut;
                changed = true;
            }

            info.isValid = true;
        }
    }

    // Build use-to-reaching-defs map
    for(std::size_t useIdx = 0; useIdx < uses.size(); ++useIdx)
    {
        const auto& use = uses[useIdx];
        const auto& reaching = blockReachingDefs[use.blockId].inSet;

        for(int defId : reaching)
        {
            if(defId >= 0 && defId < static_cast<int>(definitions.size()))
            {
                if(definitions[defId].variable == use.variable)
                {
                    useToReachingDefs[useIdx].push_back(defId);
                }
            }
        }
    }
}

void DUAnalysisCache::updateBlockReachingDefs(int blockId)
{
    if(!cfg || blockReachingDefs.find(blockId) == blockReachingDefs.end())
        return;

    ReachingDefInfo& info = blockReachingDefs[blockId];
    std::unordered_set<int> newIn;

    const BasicBlock* bb = cfg->getBlock(blockId);
    if(bb)
    {
        for(int pred : bb->preds)
        {
            const auto& predOut = blockReachingDefs[pred].outSet;
            newIn.insert(predOut.begin(), predOut.end());
        }
    }

    info.inSet = newIn;
    info.isValid = true;
}

void DUAnalysisCache::propagateChanges(const std::unordered_set<int>& changedBlocks)
{
    for(int block : changedBlocks)
    {
        updateBlockReachingDefs(block);
    }
}

} // namespace Phase8
