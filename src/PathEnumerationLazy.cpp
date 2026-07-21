#include "PathEnumerationLazy.h"
#include "Phase8Optimizer.h"
#include <iostream>

namespace Phase8
{

PathIterator::PathIterator(const CFG* cfg, int start)
    : cfg(cfg), done(false)
{
    if(cfg && cfg->hasBlock(start))
    {
        currentPath.push_back(start);
        onPath.insert(start);
        findNextPath();
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

    if(!cfg || currentPath.empty())
    {
        done = true;
        return;
    }

    int current = currentPath.back();
    const BasicBlock* block = cfg->getBlock(current);

    if(!block || block->succs.empty())
    {
        // Backtrack
        bool foundBacktrack = false;
        while(!currentPath.empty())
        {
            int top = currentPath.back();
            currentPath.pop_back();
            onPath.erase(top);

            if(!currentPath.empty())
            {
                int pred = currentPath.back();
                const BasicBlock* predBlock = cfg->getBlock(pred);
                if(predBlock)
                {
                    for(int succ : predBlock->succs)
                    {
                        if(onPath.count(succ) == 0 && succ != top)
                        {
                            currentPath.push_back(succ);
                            onPath.insert(succ);
                            foundBacktrack = true;
                            break;
                        }
                    }
                }

                if(foundBacktrack)
                    break;
            }
        }

        if(!foundBacktrack)
        {
            done = true;
        }
    }
    else
    {
        // Find next unvisited successor
        bool found = false;
        for(int succ : block->succs)
        {
            if(onPath.count(succ) == 0)
            {
                currentPath.push_back(succ);
                onPath.insert(succ);
                found = true;
                break;
            }
        }

        if(!found)
        {
            findNextPath();  // Recurse to backtrack
        }
    }
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
