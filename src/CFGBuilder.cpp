#include "CFGBuilder.h"

namespace ModernPDE
{

CFGBuilder::CFGBuilder()
    : entryBlock_(-1),
      exitBlock_(-1),
      currentFunctionEntry_(-1),
      currentFunctionExit_(-1)
{
}

CFG CFGBuilder::build(ASTNode* root)
{
    cfg_ = CFG();

    entryBlock_ = createBlock();
    exitBlock_  = createBlock();

    if(root == nullptr)
        return cfg_;

    buildTranslationUnit(root);

    return cfg_;
}

void CFGBuilder::buildTranslationUnit(ASTNode* root)
{
    for(ASTNode* child : root->children)
    {
        if(child == nullptr)
            continue;

        if(child->type == ASTNodeType::Function)
            buildFunction(child);
    }
}

int CFGBuilder::createBlock()
{
    return cfg_.createBlock();
}

void CFGBuilder::connect(
    int from,
    int to)
{
    if(from >= 0 && to >= 0)
        cfg_.addEdge(from,to);
}

void CFGBuilder::appendNode(
    int block,
    ASTNode* node)
{
    BasicBlock* B = cfg_.getBlock(block);

    if(B == nullptr || node == nullptr)
        return;

    B->instructions.push_back(
        static_cast<int>(node->type));
}


//part 2
void CFGBuilder::buildFunction(
    ASTNode* function)
{
    currentFunctionEntry_ = createBlock();
    currentFunctionExit_  = createBlock();

    connect(entryBlock_, currentFunctionEntry_);

    int currentBlock = currentFunctionEntry_;

    for(ASTNode* child : function->children)
    {
        if(child->type == ASTNodeType::CompoundStatement)
        {
            currentBlock =
                buildCompound(
                    child,
                    currentBlock);
        }
    }

    connect(currentBlock,
            currentFunctionExit_);

    connect(currentFunctionExit_,
            exitBlock_);
}
int CFGBuilder::buildCompound(
    ASTNode* compound,
    int currentBlock)
{
    for(ASTNode* child : compound->children)
    {
        currentBlock =
            buildStatement(
                child,
                currentBlock);
    }

    return currentBlock;
}
int CFGBuilder::buildStatement(
    ASTNode* stmt,
    int currentBlock)
{
    appendNode(
        currentBlock,
        stmt);

    switch(stmt->type)
    {
    case ASTNodeType::IfStatement:
        return buildIf(
            stmt,
            currentBlock);

    case ASTNodeType::WhileStatement:
        return buildWhile(
            stmt,
            currentBlock);

    case ASTNodeType::ForStatement:
        return buildFor(
            stmt,
            currentBlock);

    case ASTNodeType::ReturnStatement:
        return buildReturn(
            stmt,
            currentBlock);

    case ASTNodeType::Variable:
          return buildDeclaration(
        stmt,
        currentBlock);

    case ASTNodeType::Expression:
         return buildExpression(
        stmt,
        currentBlock);

    default:
        return currentBlock;
    }
}
int CFGBuilder::buildIf(ASTNode*, int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildWhile(ASTNode*, int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildFor(ASTNode*, int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildReturn(ASTNode*, int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildDeclaration(ASTNode*, int currentBlock)
{
    return currentBlock;
}

int CFGBuilder::buildExpression(ASTNode*, int currentBlock)
{
    return currentBlock;
}
}