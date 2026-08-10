#pragma once

#include <vector>
#include <iostream>

struct BasicBlock
{
    int id;

    std::vector<int> preds;

    std::vector<int> succs;

    std::vector<int> instructions;
};

class CFG
{
public:

    int createBlock();

    void addEdge(
        int from,
        int to);

    bool hasBlock(
        int id) const;

    BasicBlock* getBlock(
        int id);

    const BasicBlock* getBlock(
        int id) const;

    const std::vector<BasicBlock>&
    getBlocks() const;

    void print() const;

    std::size_t size() const;

    std::size_t edgeCount() const;

private:

    std::vector<BasicBlock> blocks;
};