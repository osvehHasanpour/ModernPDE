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
    blockStatements_.clear();
    functions_.clear();
    loopStack_.clear();

    entryBlock_ = createBlock();
    exitBlock_  = createBlock();

    if(root == nullptr)
    {
        return cfg_;
    }

    buildTranslationUnit(root);

    return cfg_;
}

int CFGBuilder::entryBlock() const
{
    return entryBlock_;
}

int CFGBuilder::exitBlock() const
{
    return exitBlock_;
}

const std::unordered_map<
    int,
    std::vector<ASTNode*>>&
CFGBuilder::blockStatements() const
{
    return blockStatements_;
}

const std::vector<FunctionRegion>&
CFGBuilder::functions() const
{
    return functions_;
}

void CFGBuilder::buildTranslationUnit(ASTNode* root)
{
    for(ASTNode* child : root->children)
    {
        if(child == nullptr)
        {
            continue;
        }

        if(child->type == ASTNodeType::Function)
        {
            buildFunction(child);
        }
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
    {
        cfg_.addEdge(from, to);
    }
}

void CFGBuilder::appendNode(
    int block,
    ASTNode* node)
{
    BasicBlock* B = cfg_.getBlock(block);

    if(B == nullptr || node == nullptr)
    {
        return;
    }

    B->instructions.push_back(node->line);
    blockStatements_[block].push_back(node);
}

void CFGBuilder::pushLoop(
    int breakTarget,
    int continueTarget)
{
    loopStack_.push_back(
    {
        breakTarget,
        continueTarget
    });
}

void CFGBuilder::popLoop()
{
    if(!loopStack_.empty())
    {
        loopStack_.pop_back();
    }
}

void CFGBuilder::buildFunction(ASTNode* function)
{
    currentFunctionEntry_ = createBlock();
    currentFunctionExit_  = createBlock();

    FunctionRegion region;
    region.name       = function->name;
    region.entryBlock = currentFunctionEntry_;
    region.exitBlock  = currentFunctionExit_;
    functions_.push_back(region);

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

    if(currentBlock >= 0)
    {
        connect(
            currentBlock,
            currentFunctionExit_);
    }

    connect(
        currentFunctionExit_,
        exitBlock_);
}

int CFGBuilder::buildCompound(
    ASTNode* compound,
    int currentBlock)
{
    if(currentBlock < 0)
    {
        return -1;
    }

    for(ASTNode* child : compound->children)
    {
        currentBlock =
            buildStatement(
                child,
                currentBlock);
    }

    return currentBlock;
}

int CFGBuilder::buildCompoundOrStmt(
    ASTNode* node,
    int currentBlock)
{
    if(node == nullptr)
    {
        return currentBlock;
    }

    if(node->type == ASTNodeType::CompoundStatement)
    {
        return buildCompound(
            node,
            currentBlock);
    }

    return buildStatement(
        node,
        currentBlock);
}

int CFGBuilder::buildStatement(
    ASTNode* stmt,
    int currentBlock)
{
    if(currentBlock < 0 || stmt == nullptr)
    {
        return -1;
    }

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

    case ASTNodeType::DoWhileStatement:
        return buildDoWhile(
            stmt,
            currentBlock);

    case ASTNodeType::ForStatement:
        return buildFor(
            stmt,
            currentBlock);

    case ASTNodeType::SwitchStatement:
        return buildSwitch(
            stmt,
            currentBlock);

    case ASTNodeType::BreakStatement:
        return buildBreak(
            stmt,
            currentBlock);

    case ASTNodeType::ContinueStatement:
        return buildContinue(
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

    case ASTNodeType::CompoundStatement:
        return buildCompound(
            stmt,
            currentBlock);

    default:
        appendNode(
            currentBlock,
            stmt);
        return currentBlock;
    }
}

int CFGBuilder::buildIf(
    ASTNode* node,
    int currentBlock)
{
    if(!node->children.empty())
    {
        appendNode(
            currentBlock,
            node->children[0]);
    }

    int thenBlock = createBlock();
    int mergeBlock = createBlock();

    connect(currentBlock, thenBlock);

    int thenEnd = currentBlock;

    if(node->children.size() >= 2)
    {
        thenEnd =
            buildCompoundOrStmt(
                node->children[1],
                thenBlock);
    }

    if(thenEnd >= 0)
    {
        connect(thenEnd, mergeBlock);
    }

    if(node->children.size() >= 3)
    {
        int elseBlock = createBlock();
        connect(currentBlock, elseBlock);

        int elseEnd =
            buildCompoundOrStmt(
                node->children[2],
                elseBlock);

        if(elseEnd >= 0)
        {
            connect(elseEnd, mergeBlock);
        }
    }
    else
    {
        connect(currentBlock, mergeBlock);
    }

    return mergeBlock;
}

int CFGBuilder::buildWhile(
    ASTNode* node,
    int currentBlock)
{
    int headerBlock = createBlock();
    connect(currentBlock, headerBlock);

    if(!node->children.empty())
    {
        appendNode(
            headerBlock,
            node->children[0]);
    }

    int bodyBlock = createBlock();
    int exitBlock = createBlock();

    connect(headerBlock, bodyBlock);
    connect(headerBlock, exitBlock);

    pushLoop(exitBlock, headerBlock);

    int bodyEnd = headerBlock;

    if(node->children.size() >= 2)
    {
        bodyEnd =
            buildCompoundOrStmt(
                node->children[1],
                bodyBlock);
    }

    popLoop();

    if(bodyEnd >= 0)
    {
        connect(bodyEnd, headerBlock);
    }

    return exitBlock;
}

int CFGBuilder::buildDoWhile(
    ASTNode* node,
    int currentBlock)
{
    int bodyBlock = createBlock();
    connect(currentBlock, bodyBlock);

    int headerBlock = createBlock();
    int exitBlock = createBlock();

    pushLoop(exitBlock, headerBlock);

    int bodyEnd = bodyBlock;

    if(!node->children.empty())
    {
        bodyEnd =
            buildCompoundOrStmt(
                node->children[0],
                bodyBlock);
    }

    popLoop();

    if(bodyEnd >= 0)
    {
        connect(bodyEnd, headerBlock);
    }
    else
    {
        connect(bodyBlock, headerBlock);
    }

    if(node->children.size() >= 2)
    {
        appendNode(
            headerBlock,
            node->children[1]);
    }

    connect(headerBlock, bodyBlock);
    connect(headerBlock, exitBlock);

    return exitBlock;
}

int CFGBuilder::buildFor(
    ASTNode* node,
    int currentBlock)
{
    if(!node->children.empty())
    {
        appendNode(
            currentBlock,
            node->children[0]);
    }

    int condBlock = createBlock();
    connect(currentBlock, condBlock);

    int bodyBlock = createBlock();
    int updateBlock = createBlock();
    int exitBlock = createBlock();

    connect(condBlock, bodyBlock);
    connect(condBlock, exitBlock);

    pushLoop(exitBlock, updateBlock);

    int bodyEnd = condBlock;

    if(node->children.size() >= 2)
    {
        bodyEnd =
            buildCompoundOrStmt(
                node->children[1],
                bodyBlock);
    }

    popLoop();

    if(bodyEnd >= 0)
    {
        connect(bodyEnd, updateBlock);
    }
    else
    {
        connect(bodyBlock, updateBlock);
    }

    connect(updateBlock, condBlock);

    return exitBlock;
}

int CFGBuilder::buildSwitch(
    ASTNode* node,
    int currentBlock)
{
    if(!node->children.empty())
    {
        appendNode(
            currentBlock,
            node->children[0]);
    }

    int bodyBlock = createBlock();
    int mergeBlock = createBlock();

    connect(currentBlock, bodyBlock);

    pushLoop(mergeBlock, bodyBlock);

    int bodyEnd = bodyBlock;

    if(node->children.size() >= 2)
    {
        bodyEnd =
            buildCompound(
                node->children[1],
                bodyBlock);
    }

    popLoop();

    if(bodyEnd >= 0)
    {
        connect(bodyEnd, mergeBlock);
    }

    return mergeBlock;
}

int CFGBuilder::buildBreak(
    ASTNode* node,
    int currentBlock)
{
    appendNode(
        currentBlock,
        node);

    if(!loopStack_.empty())
    {
        connect(
            currentBlock,
            loopStack_.back().breakTarget);
    }

    return -1;
}

int CFGBuilder::buildContinue(
    ASTNode* node,
    int currentBlock)
{
    appendNode(
        currentBlock,
        node);

    if(!loopStack_.empty())
    {
        connect(
            currentBlock,
            loopStack_.back().continueTarget);
    }

    return -1;
}

int CFGBuilder::buildReturn(
    ASTNode* node,
    int currentBlock)
{
    appendNode(
        currentBlock,
        node);

    connect(
        currentBlock,
        currentFunctionExit_);

    return -1;
}

int CFGBuilder::buildDeclaration(
    ASTNode* node,
    int currentBlock)
{
    appendNode(
        currentBlock,
        node);

    return currentBlock;
}

int CFGBuilder::buildExpression(
    ASTNode* node,
    int currentBlock)
{
    appendNode(
        currentBlock,
        node);

    return currentBlock;
}

}
