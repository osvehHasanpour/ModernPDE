#include "DominatorAnalysis.h"
#include "Phase8Optimizer.h"
#include <algorithm>

namespace Phase8
{

void DominatorAnalysis::compute(const CFG& cfg, int entryBlock)
{
    this->cfg = &cfg;
    domTree.clear();
    dominanceSetCache.clear();
    dominanceFrontierCache.clear();
    stats = Stats();

    // Initialize dominator nodes for all blocks
    for(const auto& block : cfg.getBlocks())
    {
        DominatorNode node;
        node.blockId = block.id;
        domTree[block.id] = node;
    }

    iterativeComputation(entryBlock);
    buildDominanceFrontiers();
}

bool DominatorAnalysis::dominates(int a, int b) const
{
    if(a == b)
        return true;

    auto it = dominanceSetCache.find(b);
    if(it != dominanceSetCache.end())
    {
        return it->second.count(a) != 0;
    }

    return false;
}

int DominatorAnalysis::getImmediateDominator(int blockId) const
{
    auto it = domTree.find(blockId);
    if(it != domTree.end())
    {
        return it->second.immediatelyDominates;
    }
    return -1;
}

const std::vector<int>& DominatorAnalysis::getDominatedBlocks(int blockId) const
{
    auto it = domTree.find(blockId);
    if(it != domTree.end())
    {
        return it->second.dominates;
    }
    static const std::vector<int> empty;
    return empty;
}

const std::vector<int>& DominatorAnalysis::getDominatingBlocks(int blockId) const
{
    auto it = domTree.find(blockId);
    if(it != domTree.end())
    {
        return it->second.dominatedBy;
    }
    static const std::vector<int> empty;
    return empty;
}

const std::unordered_set<int>& DominatorAnalysis::getDominanceFrontier(int blockId) const
{
    auto it = dominanceFrontierCache.find(blockId);
    if(it != dominanceFrontierCache.end())
    {
        return it->second;
    }
    static const std::unordered_set<int> empty;
    return empty;
}

std::vector<int> DominatorAnalysis::getMustPathBlocks(int target) const
{
    std::vector<int> mustPath;
    if(cfg == nullptr)
        return mustPath;

    // All blocks that dominate target are on every path to target
    auto it = dominanceSetCache.find(target);
    if(it != dominanceSetCache.end())
    {
        mustPath.assign(it->second.begin(), it->second.end());
        std::sort(mustPath.begin(), mustPath.end());
        stats.pathsPruned += mustPath.size();
    }

    return mustPath;
}

void DominatorAnalysis::iterativeComputation(int entryBlock)
{
    if(!cfg || !cfg->hasBlock(entryBlock))
        return;

    // Initialize: entry dominates only itself
    std::unordered_map<int, std::unordered_set<int>> doms;
    for(const auto& block : cfg->getBlocks())
    {
        doms[block.id].insert(block.id);
    }

    // Entry block dominates itself
    doms[entryBlock].clear();
    doms[entryBlock].insert(entryBlock);

    // All other blocks initially dominated by all blocks
    std::unordered_set<int> allBlocks;
    for(const auto& block : cfg->getBlocks())
    {
        allBlocks.insert(block.id);
    }

    for(const auto& block : cfg->getBlocks())
    {
        if(block.id != entryBlock)
        {
            doms[block.id] = allBlocks;
        }
    }

    // Iterative fixed-point
    bool changed = true;
    while(changed && stats.iterationsNeeded < 1000)
    {
        changed = false;
        stats.iterationsNeeded++;

        for(const auto& block : cfg->getBlocks())
        {
            if(block.id == entryBlock)
                continue;

            std::unordered_set<int> newDom;
            newDom.insert(block.id);

            if(!block.preds.empty())
            {
                // Intersection of predecessors' dominators
                std::unordered_set<int> predIntersection = doms[block.preds[0]];
                for(std::size_t i = 1; i < block.preds.size(); ++i)
                {
                    std::unordered_set<int> temp;
                    for(int d : predIntersection)
                    {
                        if(doms[block.preds[i]].count(d) != 0)
                        {
                            temp.insert(d);
                        }
                    }
                    predIntersection = temp;
                }

                newDom.insert(predIntersection.begin(), predIntersection.end());
            }

            if(newDom != doms[block.id])
            {
                doms[block.id] = newDom;
                changed = true;
            }
        }
    }

    dominanceSetCache = doms;
}

void DominatorAnalysis::buildDominanceFrontiers()
{
    if(!cfg)
        return;

    for(const auto& block : cfg->getBlocks())
    {
        dominanceFrontierCache[block.id] = std::unordered_set<int>();
    }

    for(const auto& block : cfg->getBlocks())
    {
        if(block.preds.size() >= 2)
        {
            for(int p : block.preds)
            {
                int runner = p;
                while(runner != -1 && !dominates(runner, block.id))
                {
                    dominanceFrontierCache[runner].insert(block.id);
                    auto it = domTree.find(runner);
                    if(it != domTree.end())
                    {
                        runner = it->second.immediatelyDominates;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
}

} // namespace Phase8
