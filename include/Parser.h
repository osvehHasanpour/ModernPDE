#ifndef MODERNPDE_PARSER_H
#define MODERNPDE_PARSER_H

#include "Lexer.h"

#include <vector>
#include <string>

namespace ModernPDE
{

enum class ASTNodeType
{
    TranslationUnit,

    Function,
    Parameter,
    Variable,

    CompoundStatement,

    IfStatement,
    WhileStatement,
    ForStatement,
    ReturnStatement,

    Expression,

    Unknown
};

struct ASTNode
{
    ASTNodeType type;

    std::string name;

    std::vector<ASTNode*> children;

    ASTNode(
        ASTNodeType t = ASTNodeType::Unknown,
        const std::string& n = "")
        :
        type(t),
        name(n)
    {
    }

    ~ASTNode();
};

class Parser
{
public:

    Parser(
        const std::vector<Token>& tokens);

    void parse();

    ASTNode* getRoot() const;

    size_t functionCount() const;

    size_t globalVariableCount() const;

private:

    bool eof() const;

    const Token& current() const;

    const Token& peek(int offset = 1) const;

    bool match(TokenType type);

    bool match(const std::string& text);

    void advance();

    void parseTranslationUnit();

    void parseDeclaration();

    void parseFunction(
        const std::string& name);

    void parseVariable(
        const std::string& name);

    void parseCompound();

    void parseStatement();

    void parseIf();

    void parseWhile();

    void parseFor();

    void parseReturn();

private:

    std::vector<Token> tokenList_;

    size_t index_ = 0;

    ASTNode* rootNode_ = nullptr;

    std::vector<ASTNode*> functionList_;

    std::vector<ASTNode*> globalVariableList_;
};

}

#endif