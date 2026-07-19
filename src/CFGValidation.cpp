#include "CFGValidation.h"

#include <iostream>
#include <queue>
#include <unordered_set>

CFGValidation::Report CFGValidation::validate(
    const CFG& cfg,
    int entryBlock,
    const std::vector<CFGFunctionBounds>& functions)
{
    Report report;

    if(!cfg.hasBlock(entryBlock))
    {
        report.valid = false;
        report.errors.push_back(
            "Entry block does not exist");
        return report;
    }

    std::unordered_set<int> reachable;
    std::queue<int> worklist;

    worklist.push(entryBlock);
    reachable.insert(entryBlock);

    while(!worklist.empty())
    {
        int current = worklist.front();
        worklist.pop();

        const BasicBlock* block =
            cfg.getBlock(current);

        if(block == nullptr)
        {
            continue;
        }

        std::unordered_set<int> seenSuccs;

        for(int succ : block->succs)
        {
            if(seenSuccs.count(succ) != 0)
            {
                report.valid = false;
                report.errors.push_back(
                    "Duplicate edge "
                    + std::to_string(current)
                    + " -> "
                    + std::to_string(succ));
            }

            seenSuccs.insert(succ);

            if(!cfg.hasBlock(succ))
            {
                report.valid = false;
                report.errors.push_back(
                    "Broken edge "
                    + std::to_string(current)
                    + " -> "
                    + std::to_string(succ));
                continue;
            }

            if(reachable.insert(succ).second)
            {
                worklist.push(succ);
            }
        }
    }

    for(const auto& block : cfg.getBlocks())
    {
        if(reachable.find(block.id) ==
           reachable.end())
        {
            report.unreachable.push_back(
                block.id);
            report.valid = false;
        }

        if(block.preds.empty() &&
           block.succs.empty())
        {
            report.isolated.push_back(
                block.id);
            report.valid = false;
        }

        for(int succ : block.succs)
        {
            const BasicBlock* succBlock =
                cfg.getBlock(succ);

            if(succBlock == nullptr)
            {
                continue;
            }

            bool found = false;

            for(int pred : succBlock->preds)
            {
                if(pred == block.id)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                report.valid = false;
                report.errors.push_back(
                    "Asymmetric edge "
                    + std::to_string(block.id)
                    + " -> "
                    + std::to_string(succ));
            }
        }
    }

    for(const auto& function : functions)
    {
        Report::FunctionReport fnReport;
        fnReport.name       = function.name;
        fnReport.entryBlock = function.entryBlock;
        fnReport.exitBlock  = function.exitBlock;

        const BasicBlock* entry =
            cfg.getBlock(function.entryBlock);

        const BasicBlock* exit =
            cfg.getBlock(function.exitBlock);

        if(entry == nullptr)
        {
            report.valid = false;
            fnReport.validEntry = false;
            report.errors.push_back(
                "Function "
                + function.name
                + " has invalid entry block");
        }
        else if(entry->preds.size() != 1)
        {
            report.valid = false;
            fnReport.validEntry = false;
            report.errors.push_back(
                "Function "
                + function.name
                + " entry block must have "
                  "exactly one predecessor");
        }

        if(exit == nullptr)
        {
            report.valid = false;
            fnReport.validExit = false;
            report.errors.push_back(
                "Function "
                + function.name
                + " has invalid exit block");
        }
        else if(exit->succs.size() != 1)
        {
            report.valid = false;
            fnReport.validExit = false;
            report.errors.push_back(
                "Function "
                + function.name
                + " exit block must have "
                  "exactly one successor");
        }

        report.functions.push_back(fnReport);
    }

    return report;
}

void CFGValidation::print(
    const Report& report)
{
    std::cout
    << "\n====================\n";

    std::cout
    << "CFG VALIDATION\n";

    std::cout
    << "====================\n";

    std::cout
    << "Valid : "
    << (report.valid ? "YES" : "NO")
    << "\n";

    std::cout
    << "Unreachable blocks : "
    << report.unreachable.size()
    << "\n";

    for(int id : report.unreachable)
    {
        std::cout
        << "  Block "
        << id
        << "\n";
    }

    std::cout
    << "Isolated blocks : "
    << report.isolated.size()
    << "\n";

    for(int id : report.isolated)
    {
        std::cout
        << "  Block "
        << id
        << "\n";
    }

    if(!report.functions.empty())
    {
        std::cout
        << "Function checks : "
        << report.functions.size()
        << "\n";

        for(const auto& fn : report.functions)
        {
            std::cout
            << "  "
            << fn.name
            << " entry="
            << fn.entryBlock
            << " exit="
            << fn.exitBlock
            << " entryOK="
            << (fn.validEntry ? "YES" : "NO")
            << " exitOK="
            << (fn.validExit ? "YES" : "NO")
            << "\n";
        }
    }

    std::cout
    << "Errors : "
    << report.errors.size()
    << "\n";

    for(const auto& error : report.errors)
    {
        std::cout
        << "  "
        << error
        << "\n";
    }
}
