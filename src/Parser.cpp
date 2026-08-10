#include "Parser.h"

#include <iostream>
#include <unordered_set>

namespace ModernPDE
{

namespace
{

bool isTypeKeyword(
    const std::string& word)
{
    static const std::unordered_set<std::string>
        types =
    {
        "void",
        "bool",
        "char",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
        "auto"
    };

    return types.count(word) != 0;
}

}

ASTNode::~ASTNode()
{
    for(ASTNode* child : children)
    {
        delete child;
    }
}

Parser::Parser(const std::vector<Token>& tokens)
    : tokenList_(tokens),
      index_(0),
      rootNode_(nullptr)
{
}

void Parser::parse()
{
    functionList_.clear();
    globalVariableList_.clear();

    if(rootNode_)
    {
        delete rootNode_;
    }

    rootNode_ = nullptr;
    rootNode_ =
        new ASTNode(
            ASTNodeType::TranslationUnit,
            "TranslationUnit");

    parseTranslationUnit();
}

ASTNode* Parser::getRoot() const
{
    return rootNode_;
}

size_t Parser::functionCount() const
{
    return functionList_.size();
}

size_t Parser::globalVariableCount() const
{
    return globalVariableList_.size();
}

bool Parser::eof() const
{
    return index_ >= tokenList_.size();
}

const Token& Parser::current() const
{
    return tokenList_[index_];
}

const Token& Parser::peek(int offset) const
{
    std::size_t pos = index_ + offset;

    if(pos >= tokenList_.size())
    {
        return tokenList_.back();
    }

    return tokenList_[pos];
}

bool Parser::match(TokenType type)
{
    if(eof())
    {
        return false;
    }

    return current().type == type;
}

bool Parser::match(const std::string& text)
{
    if(eof())
    {
        return false;
    }

    return current().text == text;
}

void Parser::advance()
{
    if(!eof())
    {
        index_++;
    }
}

void Parser::parseTranslationUnit()
{
    while(!eof())
    {
        parseDeclaration();
    }
}

void Parser::parseDeclaration()
{
    if(index_ + 2 >= tokenList_.size())
    {
        advance();
        return;
    }

    if(current().type != TokenType::Keyword)
    {
        advance();
        return;
    }

    std::string typeName = current().text;
    advance();

    if(!match(TokenType::Identifier))
    {
        return;
    }

    std::string name = current().text;
    advance();

    if(match("("))
    {
        parseFunction(name);
        return;
    }

    parseVariable(name);
}

void Parser::parseFunction(const std::string& name)
{
    int line = current().line;

    int depth = 0;

    while(!eof())
    {
        if(match("("))
        {
            depth++;
        }
        else if(match(")"))
        {
            depth--;

            if(depth == 0)
            {
                advance();
                break;
            }
        }

        advance();
    }

    if(match(";"))
    {
        advance();
        return;
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::Function,
            name);

    node->line = line;
    functionList_.push_back(node);
    rootNode_->children.push_back(node);

    if(match("{"))
    {
        ASTNode* body =
            new ASTNode(
                ASTNodeType::CompoundStatement,
                "");

        body->line = current().line;
        parseCompound(body);
        node->children.push_back(body);
    }
}

void Parser::parseVariable(const std::string& name)
{
    ASTNode* node =
        new ASTNode(
            ASTNodeType::Variable,
            name);

    node->line = current().line;
    globalVariableList_.push_back(node);
    rootNode_->children.push_back(node);

    while(!eof())
    {
        if(match(";"))
        {
            advance();
            break;
        }

        advance();
    }
}

void Parser::parseCompound(ASTNode* compound)
{
    if(match("{"))
    {
        advance();
    }

    while(!eof() && !match("}"))
    {
        if(match("case") || match("default"))
        {
            skipCaseLabel();
            continue;
        }

        ASTNode* stmt = parseStatement();

        if(stmt != nullptr)
        {
            compound->children.push_back(stmt);
        }
    }

    if(match("}"))
    {
        advance();
    }
}

ASTNode* Parser::parseStatement()
{
    if(eof())
    {
        return nullptr;
    }

    if(match("if"))
    {
        return parseIfStatement();
    }

    if(match("while"))
    {
        return parseWhileStatement();
    }

    if(match("do"))
    {
        return parseDoWhileStatement();
    }

    if(match("for"))
    {
        return parseForStatement();
    }

    if(match("switch"))
    {
        return parseSwitchStatement();
    }

    if(match("break"))
    {
        return parseBreakStatement();
    }

    if(match("continue"))
    {
        return parseContinueStatement();
    }

    if(match("return"))
    {
        return parseReturnStatement();
    }

    if(match("{"))
    {
        return parseCompoundStatement();
    }

    if(match(TokenType::Keyword) &&
       isTypeKeyword(current().text))
    {
        return parseLocalDeclaration();
    }

    if(match(TokenType::Identifier))
    {
        return parseExpressionStatement();
    }

    skipStatement();
    return nullptr;
}

ASTNode* Parser::parseIfStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::IfStatement,
            cond);

    node->line = line;

    ASTNode* condNode =
        new ASTNode(
            ASTNodeType::Expression,
            cond);

    condNode->line = line;
    node->children.push_back(condNode);

    ASTNode* thenBranch = parseStatement();

    if(thenBranch != nullptr)
    {
        node->children.push_back(thenBranch);
    }

    if(match("else"))
    {
        advance();

        ASTNode* elseBranch = parseStatement();

        if(elseBranch != nullptr)
        {
            node->children.push_back(elseBranch);
        }
    }

    return node;
}

ASTNode* Parser::parseWhileStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::WhileStatement,
            cond);

    node->line = line;

    ASTNode* condNode =
        new ASTNode(
            ASTNodeType::Expression,
            cond);

    condNode->line = line;
    node->children.push_back(condNode);

    ASTNode* body = parseStatement();

    if(body != nullptr)
    {
        node->children.push_back(body);
    }

    return node;
}

ASTNode* Parser::parseForStatement()
{
    int line = current().line;
    advance();

    std::string header;

    if(match("("))
    {
        header = collectBalanced('(', ')');
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::ForStatement,
            header);

    node->line = line;

    ASTNode* headerNode =
        new ASTNode(
            ASTNodeType::Expression,
            header);

    headerNode->line = line;
    node->children.push_back(headerNode);

    ASTNode* body = parseStatement();

    if(body != nullptr)
    {
        node->children.push_back(body);
    }

    return node;
}

ASTNode* Parser::parseDoWhileStatement()
{
    int line = current().line;
    advance();

    ASTNode* node =
        new ASTNode(
            ASTNodeType::DoWhileStatement,
            "");

    node->line = line;

    ASTNode* body = parseStatement();

    if(body != nullptr)
    {
        node->children.push_back(body);
    }

    if(match("while"))
    {
        advance();

        std::string cond;

        if(match("("))
        {
            cond = collectBalanced('(', ')');
        }

        ASTNode* condNode =
            new ASTNode(
                ASTNodeType::Expression,
                cond);

        condNode->line = current().line;
        node->children.push_back(condNode);
    }

    if(match(";"))
    {
        advance();
    }

    return node;
}

ASTNode* Parser::parseSwitchStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::SwitchStatement,
            cond);

    node->line = line;

    ASTNode* condNode =
        new ASTNode(
            ASTNodeType::Expression,
            cond);

    condNode->line = line;
    node->children.push_back(condNode);

    ASTNode* body = parseCompoundStatement();

    if(body != nullptr)
    {
        node->children.push_back(body);
    }

    return node;
}

ASTNode* Parser::parseBreakStatement()
{
    int line = current().line;
    advance();

    if(match(";"))
    {
        advance();
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::BreakStatement,
            "break");

    node->line = line;
    return node;
}

ASTNode* Parser::parseContinueStatement()
{
    int line = current().line;
    advance();

    if(match(";"))
    {
        advance();
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::ContinueStatement,
            "continue");

    node->line = line;
    return node;
}

ASTNode* Parser::parseReturnStatement()
{
    int line = current().line;
    advance();

    std::string expr =
        collectUntilSemicolon();

    if(match(";"))
    {
        advance();
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::ReturnStatement,
            expr);

    node->line = line;
    return node;
}

ASTNode* Parser::parseCompoundStatement()
{
    ASTNode* compound =
        new ASTNode(
            ASTNodeType::CompoundStatement,
            "");

    compound->line = current().line;
    parseCompound(compound);
    return compound;
}

ASTNode* Parser::parseLocalDeclaration()
{
    int line = current().line;
    advance();

    if(!match(TokenType::Identifier))
    {
        skipStatement();
        return nullptr;
    }

    std::string name = current().text;
    advance();

    ASTNode* node =
        new ASTNode(
            ASTNodeType::Variable,
            name);

    node->line = line;

    if(match("="))
    {
        advance();

        std::string init =
            collectUntilSemicolon();

        ASTNode* initExpr =
            new ASTNode(
                ASTNodeType::Expression,
                init);

        initExpr->line = line;
        node->children.push_back(initExpr);
    }

    if(match(";"))
    {
        advance();
    }

    return node;
}

ASTNode* Parser::parseExpressionStatement()
{
    int line = current().line;

    std::string text =
        collectUntilSemicolon();

    if(match(";"))
    {
        advance();
    }

    ASTNode* node =
        new ASTNode(
            ASTNodeType::Expression,
            text);

    node->line = line;
    return node;
}

std::string Parser::collectBalanced(
    char open,
    char close)
{
    std::string result;

    if(!match(std::string(1, open)))
    {
        return result;
    }

    int depth = 0;

    while(!eof())
    {
        const std::string& text =
            current().text;

        if(text == std::string(1, open))
        {
            depth++;
        }
        else if(text == std::string(1, close))
        {
            depth--;

            if(depth == 0)
            {
                result += text;
                advance();
                break;
            }
        }

        result += text;
        advance();
    }

    return result;
}

std::string Parser::collectUntilSemicolon()
{
    std::string result;
    int parenDepth = 0;
    int braceDepth = 0;

    while(!eof())
    {
        const std::string& text =
            current().text;

        if(text == "(")
        {
            parenDepth++;
        }
        else if(text == ")")
        {
            if(parenDepth > 0)
            {
                parenDepth--;
            }
        }
        else if(text == "{")
        {
            braceDepth++;
        }
        else if(text == "}")
        {
            if(braceDepth > 0)
            {
                braceDepth--;
            }
        }
        else if(text == ";" &&
                parenDepth == 0 &&
                braceDepth == 0)
        {
            break;
        }

        result += text;

        if(!result.empty() &&
           result.back() != ' ')
        {
            result += " ";
        }

        advance();
    }

    if(!result.empty() &&
       result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

void Parser::skipStatement()
{
    int parenDepth = 0;
    int braceDepth = 0;

    while(!eof())
    {
        if(match("("))
        {
            parenDepth++;
        }
        else if(match(")"))
        {
            if(parenDepth > 0)
            {
                parenDepth--;
            }
        }
        else if(match("{"))
        {
            braceDepth++;
        }
        else if(match("}"))
        {
            if(braceDepth > 0)
            {
                braceDepth--;
            }
            else
            {
                break;
            }
        }
        else if(match(";") &&
                parenDepth == 0 &&
                braceDepth == 0)
        {
            advance();
            break;
        }

        advance();
    }
}

void Parser::skipCaseLabel()
{
    advance();

    int parenDepth = 0;

    while(!eof())
    {
        if(match("("))
        {
            parenDepth++;
        }
        else if(match(")"))
        {
            if(parenDepth > 0)
            {
                parenDepth--;
            }
        }
        else if(match(":") && parenDepth == 0)
        {
            advance();
            break;
        }

        advance();
    }
}

}
