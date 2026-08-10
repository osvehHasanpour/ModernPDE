#pragma once

#include "CFG.h"
#include <vector>
#include <unordered_set>
#include <iterator>
#include <memory>
#include <functional>

namespace Phase8
{

/// Forward declaration
class PathEnumerationLazy;

/// Iterator for lazy path enumeration
class PathIterator
{
public:
    using value_type = std::vector<int>;
    using reference = const value_type&;
    using pointer = const value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    PathIterator() : done(true) {}

    reference operator*() const { return currentPath; }
    pointer operator->() const { return &currentPath; }

    PathIterator& operator++();
    PathIterator operator++(int);

    bool operator==(const PathIterator& other) const
    {
        return done == other.done;
    }

    bool operator!=(const PathIterator& other) const
    {
        return !(*this == other);
    }

private:
    friend class PathEnumerationLazy;

    PathIterator(const CFG* cfg, int start);

    const CFG* cfg = nullptr;
    std::vector<int> currentPath;
    // nextChildIdx[i] = index of the next not-yet-tried successor of currentPath[i].
    // Kept alongside currentPath so that backtracking resumes where it left off
    // instead of forgetting which siblings were already explored.
    std::vector<std::size_t> nextChildIdx;
    bool done = true;

    /// Advance the DFS to the next full root-to-leaf path.
    /// If `backtrackFirst` is true, the current (already-emitted) leaf path
    /// is popped before searching continues. Returns false once enumeration
    /// is exhausted.
    bool advance(bool backtrackFirst);
    void findNextPath();
};

/// Lazy path enumeration: generates paths on-demand instead of enumerating all upfront
class PathEnumerationLazy
{
public:
    PathEnumerationLazy() = default;

    /// Initialize with CFG
    void initialize(const CFG& cfg, int startBlock);

    /// Get iterator to first path
    PathIterator begin();

    /// Get end iterator
    PathIterator end();

    /// Get a specific path by index (generates paths as needed)
    std::vector<int> getPath(std::size_t index);

    /// Get total paths (requires full enumeration - should use sparingly)
    std::size_t totalPaths();

    /// Check if a path satisfies a predicate
    bool hasPathMatching(std::function<bool(const std::vector<int>&)> predicate);

    /// Enumerate paths while predicate holds
    std::vector<std::vector<int>> enumerateWhile(
        std::function<bool(const std::vector<int>&)> predicate);

    /// Get generation statistics
    struct Stats
    {
        uint64_t pathsGenerated = 0;
        uint64_t maxPathLength = 0;
        uint64_t iteratorSteps = 0;
    };

    const Stats& getStats() const { return stats; }

private:
    const CFG* cfg = nullptr;
    int startBlock = -1;
    mutable Stats stats;
};

} // namespace Phase8
