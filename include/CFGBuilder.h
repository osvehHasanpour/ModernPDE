#ifndef MODERNPDE_CFGBUILDER_H
#define MODERNPDE_CFGBUILDER_H

#include "CFG.h"
#include "Parser.h"

#include <stack>

namespace ModernPDE
{

class CFGBuilder
{
public:

    CFGBuilder();

    CFG build(ASTNode* root);

private:

    //----------------------------------------------------
    // Core
    //----------------------------------------------------

    void buildTranslationUnit(ASTNode* root);

    void buildFunction(ASTNode* function);

    int buildCompound(
        ASTNode* compound,
        int currentBlock);

    int buildStatement(
        ASTNode* stmt,
        int currentBlock);

    //----------------------------------------------------
    // Statements
    //----------------------------------------------------

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

    //----------------------------------------------------
    // Expressions
    //----------------------------------------------------

    void visitExpression(ASTNode* expr);

    //----------------------------------------------------
    // Helpers
    //----------------------------------------------------

    int createBlock();

    void connect(
        int from,
        int to);

    void appendNode(
        int block,
        ASTNode* node);

    //----------------------------------------------------
    // State
    //----------------------------------------------------

    CFG cfg_;

    int entryBlock_;

    int exitBlock_;

    int currentFunctionEntry_;

    int currentFunctionExit_;

};

}

#endif