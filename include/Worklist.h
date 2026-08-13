#pragma once

#include <cstddef>
#include <queue>
#include <unordered_set>

class Worklist
{
public:

    void push(int item);

    bool empty() const;

    int pop();

    std::size_t size() const;

    void clear();

private:

    std::queue<int> items_;

    std::unordered_set<int> membership_;
};
