#include "PathEnumerationLazy.h"
#include "Phase8Optimizer.h"
#include <algorithm>
#include <iostream>

namespace Phase8
{

PathIterator::PathIterator(const CFG* cfg, int start)
    : cfg(cfg), done(false)
{
    if(cfg && cfg->hasBlock(start))
    {
        currentPath.push_back(start);
        nextChildIdx.push_back(0);
        if(!advance(/*backtrackFirst=*/false))
        {
            done = true;
        }
    }
    else
    {
        done = true;
    }
}

PathIterator& PathIterator::operator++()
{
    findNextPath();
    return *this;
}

PathIterator PathIterator::operator++(int)
{
    PathIterator tmp = *this;
    ++(*this);
    return tmp;
}

void PathIterator::findNextPath()
{
    if(done)
        return;

    if(!advance(/*backtrackFirst=*/true))
    {
        done = true;
        currentPath.clear();
        nextChildIdx.clear();
    }
}

// Iterative DFS that always drives the path forward to a full root-to-leaf
// path. `nextChildIdx` remembers, per level, which successor to try next,
// so a level is never re-explored after backtracking through it (the
// earlier onPath-based version lost that memory once a node was popped,
// which made it re-walk the same branches forever). Successors already on
// the current path are skipped to avoid looping forever on CFG back edges.
bool PathIterator::advance(bool backtrackFirst)
{
    if(!cfg)
        return false;

    if(backtrackFirst)
    {
        // We just emitted currentPath ending at a leaf; pop it and resume
        // searching for the next path from its parent.
        if(!currentPath.empty())
        {
            currentPath.pop_back();
            nextChildIdx.pop_back();
        }
    }

    while(!currentPath.empty())
    {
        int node = currentPath.back();
        const BasicBlock* block = cfg->getBlock(node);

        if(!block || block->succs.empty())
        {
            // Leaf: currentPath is a complete root-to-leaf path.
            return true;
        }

        std::size_t& idx = nextChildIdx.back();
        bool descended = false;
        while(idx < block->succs.size())
        {
            int succ = block->succs[idx];
            ++idx;

            // Skip successors already on the path to avoid infinite loops
            // on CFG back edges (cycles).
            if(std::find(currentPath.begin(), currentPath.end(), succ) == currentPath.end())
            {
                currentPath.push_back(succ);
                nextChildIdx.push_back(0);
                descended = true;
                break;
            }
        }

        if(!descended)
        {
            // Exhausted every successor at this level; backtrack.
            currentPath.pop_back();
            nextChildIdx.pop_back();
        }
    }

    return false; // Enumeration exhausted.
}

void PathEnumerationLazy::initialize(const CFG& cfg, int startBlock)
{
    this->cfg = &cfg;
    this->startBlock = startBlock;
}

PathIterator PathEnumerationLazy::begin()
{
    return PathIterator(cfg, startBlock);
}

PathIterator PathEnumerationLazy::end()
{
    return PathIterator();
}

std::vector<int> PathEnumerationLazy::getPath(std::size_t index)
{
    std::size_t count = 0;
    for(auto it = begin(); it != end(); ++it)
    {
        if(count == index)
        {
            return *it;
        }
        count++;
    }
    return std::vector<int>();
}

std::size_t PathEnumerationLazy::totalPaths()
{
    std::size_t count = 0;
    for(auto it = begin(); it != end(); ++it)
    {
        count++;
    }
    stats.pathsGenerated = count;
    return count;
}

bool PathEnumerationLazy::hasPathMatching(std::function<bool(const std::vector<int>&)> predicate)
{
    for(auto it = begin(); it != end(); ++it)
    {
        if(predicate(*it))
        {
            return true;
        }
    }
    return false;
}

std::vector<std::vector<int>> PathEnumerationLazy::enumerateWhile(
    std::function<bool(const std::vector<int>&)> predicate)
{
    std::vector<std::vector<int>> result;
    for(auto it = begin(); it != end(); ++it)
    {
        if(predicate(*it))
        {
            result.push_back(*it);
        }
        else
        {
            break;
        }
    }
    return result;
}

} // namespace Phase8
