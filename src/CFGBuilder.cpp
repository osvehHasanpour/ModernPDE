#include "CFGBuilder.h"

#include <iostream>

namespace ModernPDE
{

CFGBuilder::CFGBuilder()
    :
    entryBlock_(-1),
    exitBlock_(-1),
    currentFunctionEntry_(-1),
    currentFunctionExit_(-1)
{
}

CFG CFGBuilder::build(ASTNode* root)
{
    cfg_ = CFG();

    entryBlock_ = createBlock();

    exitBlock_ = createBlock();

    if(root == nullptr)
    {
        return cfg_;
    }

    buildTranslationUnit(root);

    return cfg_;
}

/////////////////////////////////////////////////////

void CFGBuilder::buildTranslationUnit(
    ASTNode* root)
{
    for(ASTNode* child : root->children)
    {
        if(child == nullptr)
        {
            continue;
        }

        if(child->type ==
           ASTNodeType::Function)
        {
            buildFunction(child);
        }
    }
}

/////////////////////////////////////////////////////

int CFGBuilder::createBlock()
{
    return cfg_.createBlock();
}

/////////////////////////////////////////////////////

void CFGBuilder::connect(
    int from,
    int to)
{
    if(from < 0 || to < 0)
    {
        return;
    }

    cfg_.addEdge(
        from,
        to);
}

/////////////////////////////////////////////////////

void CFGBuilder::appendNode(
    int block,
    ASTNode* node)
{
    BasicBlock* B =
        cfg_.getBlock(block);

    if(B == nullptr)
    {
        return;
    }

    // بعداً اینجا ASTNode ذخیره می‌کنیم
    // فعلاً فقط شماره نوع نود

    B->instructions.push_back(
        static_cast<int>(node->type));
}


int CFGBuilder::buildCompound(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

void CFGBuilder::buildFunction(
    ASTNode* node)
{
}

int CFGBuilder::buildStatement(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildIf(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildWhile(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildFor(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildReturn(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildDeclaration(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildExpression(
    ASTNode* node,
    int currentBlock)
{
    return currentBlock;
}

void CFGBuilder::visitExpression(
    ASTNode* node)
{
}
}