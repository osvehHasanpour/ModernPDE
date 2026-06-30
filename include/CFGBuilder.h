#pragma once

#include "CFG.h"
#include "Parser.h"

namespace ModernPDE
{

class CFGBuilder
{
public:
    CFGBuilder();

    CFG build(ASTNode* root);

private:

    void buildTranslationUnit(ASTNode* root);

    void buildFunction(ASTNode* function);

    int buildCompound(
        ASTNode* compound,
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

    int buildFor(
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

    void visitExpression(ASTNode* expr);

    int createBlock();

    void connect(
        int from,
        int to);

    void appendNode(
        int block,
        ASTNode* node);

private:

    CFG cfg_;

    int entryBlock_;
    int exitBlock_;

    int currentFunctionEntry_;
    int currentFunctionExit_;
};

}