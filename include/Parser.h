#pragma once

#include "Lexer.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

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
    DoWhileStatement,
    ForStatement,
    SwitchStatement,
    BreakStatement,
    ContinueStatement,
    ReturnStatement,

    Expression,

    Unknown
};

struct ASTNode
{
    ASTNodeType type;

    std::string name;

    int line = 0;

    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode(
        ASTNodeType t = ASTNodeType::Unknown,
        const std::string& n = "")
        :
        type(t),
        name(n)
    {
    }

    ASTNode* child(std::size_t index) const
    {
        if(index >= children.size())
        {
            return nullptr;
        }

        return children[index].get();
    }

    std::size_t childCount() const
    {
        return children.size();
    }

    void addChild(std::unique_ptr<ASTNode> node)
    {
        if(node)
        {
            children.push_back(std::move(node));
        }
    }
};

class Parser
{
public:

    Parser(
        const std::vector<Token>& tokens);

    void parse();

    // Non-owning view; the Parser retains ownership of the AST.
    ASTNode* getRoot() const;

    size_t functionCount() const;

    size_t globalVariableCount() const;

    bool hasError() const;

    const std::string& lastError() const;

private:

    bool eof() const;

    const Token& current() const;

    const Token& peek(int offset = 1) const;

    bool match(TokenType type);

    bool match(const std::string& text);

    void advance();

    void setError(const std::string& message);

    void parseTranslationUnit();

    void parseDeclaration();

    void parseFunction(
        const std::string& name);

    void parseVariable(
        const std::string& name);

    void parseCompound(
        ASTNode* compound);

    std::unique_ptr<ASTNode> parseStatement();

    std::unique_ptr<ASTNode> parseIfStatement();

    std::unique_ptr<ASTNode> parseWhileStatement();

    std::unique_ptr<ASTNode> parseDoWhileStatement();

    std::unique_ptr<ASTNode> parseForStatement();

    std::unique_ptr<ASTNode> parseSwitchStatement();

    std::unique_ptr<ASTNode> parseBreakStatement();

    std::unique_ptr<ASTNode> parseContinueStatement();

    std::unique_ptr<ASTNode> parseReturnStatement();

    std::unique_ptr<ASTNode> parseCompoundStatement();

    std::unique_ptr<ASTNode> parseLocalDeclaration();

    std::unique_ptr<ASTNode> parseExpressionStatement();

    std::string collectBalanced(
        char open,
        char close);

    std::string collectUntilSemicolon();

    void skipStatement();

    void skipCaseLabel();

private:

    std::vector<Token> tokenList_;

    size_t index_ = 0;

    std::unique_ptr<ASTNode> rootNode_;

    std::vector<ASTNode*> functionList_;

    std::vector<ASTNode*> globalVariableList_;

    std::string lastError_;
};

}
