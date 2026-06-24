#include "CFG.h"

#include <iostream>

int CFG::createBlock()
{
    BasicBlock block;

    block.id =
        static_cast<int>(
            blocks.size());

    blocks.push_back(block);

    return block.id;
}

void CFG::addEdge(
    int from,
    int to)
{
    if(!hasBlock(from) ||
       !hasBlock(to))
    {
        return;
    }

    blocks[from]
        .succs
        .push_back(to);

    blocks[to]
        .preds
        .push_back(from);
}

bool CFG::hasBlock(
    int id) const
{
    return
        id >= 0 &&
        id < static_cast<int>(
            blocks.size());
}

BasicBlock* CFG::getBlock(
    int id)
{
    if(!hasBlock(id))
    {
        return nullptr;
    }

    return &blocks[id];
}

const BasicBlock* CFG::getBlock(
    int id) const
{
    if(!hasBlock(id))
    {
        return nullptr;
    }

    return &blocks[id];
}

const std::vector<BasicBlock>&
CFG::getBlocks() const
{
    return blocks;
}

std::size_t CFG::size() const
{
    return blocks.size();
}

void CFG::print() const
{
    std::cout
    << "\n====================\n";

    std::cout
    << "CONTROL FLOW GRAPH\n";

    std::cout
    << "====================\n";

    for(const auto& block : blocks)
    {
        std::cout
        << "\nBlock "
        << block.id
        << "\n";

        std::cout
        << "Preds: ";

        for(auto p : block.preds)
        {
            std::cout
            << p
            << " ";
        }

        std::cout
        << "\n";

        std::cout
        << "Succs: ";

        for(auto s : block.succs)
        {
            std::cout
            << s
            << " ";
        }

        std::cout
        << "\n";
    }
}