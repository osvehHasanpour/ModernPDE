#pragma once

#include "CFG.h"
#include "Parser.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace ModernPDE
{

struct FunctionRegion
{
    std::string name;

    int entryBlock = -1;

    int exitBlock = -1;
};

class CFGBuilder
{
public:
    CFGBuilder();

    CFG build(ASTNode* root);

    int entryBlock() const;

    int exitBlock() const;

    const std::unordered_map<
        int,
        std::vector<ASTNode*>>&
    blockStatements() const;

    const std::vector<FunctionRegion>&
    functions() const;

private:

    struct LoopContext
    {
        int breakTarget = -1;

        int continueTarget = -1;
    };

    void buildTranslationUnit(ASTNode* root);

    void buildFunction(ASTNode* function);

    int buildCompound(
        ASTNode* compound,
        int currentBlock);

    int buildCompoundOrStmt(
        ASTNode* node,
        int currentBlock);

    int buildStatement(
        ASTNode* stmt,
        int currentBlock);

    int buildIf(
        ASTNode* node,
        int currentBlock);

    int buildWhile(
        ASTNode* node,
        int currentBlock);

    int buildDoWhile(
        ASTNode* node,
        int currentBlock);

    int buildFor(
        ASTNode* node,
        int currentBlock);

    int buildSwitch(
        ASTNode* node,
        int currentBlock);

    int buildBreak(
        ASTNode* node,
        int currentBlock);

    int buildContinue(
        ASTNode* node,
        int currentBlock);

    int buildReturn(
        ASTNode* node,
        int currentBlock);

    int buildDeclaration(
        ASTNode* node,
        int currentBlock);

    int buildExpression(
        ASTNode* node,
        int currentBlock);

    int createBlock();

    void connect(
        int from,
        int to);

    void appendNode(
        int block,
        ASTNode* node);

    void pushLoop(
        int breakTarget,
        int continueTarget);

    void popLoop();

private:

    CFG cfg_;

    int entryBlock_;
    int exitBlock_;

    int currentFunctionEntry_;
    int currentFunctionExit_;

    std::unordered_map<
        int,
        std::vector<ASTNode*>> blockStatements_;

    std::vector<FunctionRegion> functions_;

    std::vector<LoopContext> loopStack_;
};

}
