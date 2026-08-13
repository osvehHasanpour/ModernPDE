#include "Parser.h"

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

std::unique_ptr<ASTNode> makeNode(
    ASTNodeType type,
    const std::string& name,
    int line)
{
    auto node = std::make_unique<ASTNode>(type, name);
    node->line = line;
    return node;
}

const Token& eofToken()
{
    static const Token token{
        TokenType::EndOfFile,
        "",
        0,
        0};

    return token;
}

}

Parser::Parser(const std::vector<Token>& tokens)
    : tokenList_(tokens),
      index_(0)
{
}

void Parser::parse()
{
    functionList_.clear();
    globalVariableList_.clear();
    lastError_.clear();
    index_ = 0;

    rootNode_ =
        std::make_unique<ASTNode>(
            ASTNodeType::TranslationUnit,
            "TranslationUnit");

    parseTranslationUnit();
}

ASTNode* Parser::getRoot() const
{
    return rootNode_.get();
}

size_t Parser::functionCount() const
{
    return functionList_.size();
}

size_t Parser::globalVariableCount() const
{
    return globalVariableList_.size();
}

bool Parser::hasError() const
{
    return !lastError_.empty();
}

const std::string& Parser::lastError() const
{
    return lastError_;
}

bool Parser::eof() const
{
    return index_ >= tokenList_.size();
}

const Token& Parser::current() const
{
    if(eof())
    {
        return eofToken();
    }

    return tokenList_[index_];
}

const Token& Parser::peek(int offset) const
{
    if(tokenList_.empty())
    {
        return eofToken();
    }

    if(offset < 0)
    {
        const std::size_t back =
            static_cast<std::size_t>(-offset);

        if(back > index_)
        {
            return eofToken();
        }

        return tokenList_[index_ - back];
    }

    const std::size_t pos =
        index_ + static_cast<std::size_t>(offset);

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

void Parser::setError(const std::string& message)
{
    if(lastError_.empty())
    {
        lastError_ = message;
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
    bool closed = false;

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
                closed = true;
                break;
            }
        }

        advance();
    }

    if(!closed)
    {
        setError(
            "Unterminated parameter list for function '" +
            name +
            "'");
        return;
    }

    if(match(";"))
    {
        advance();
        return;
    }

    auto node =
        makeNode(
            ASTNodeType::Function,
            name,
            line);

    ASTNode* raw = node.get();
    functionList_.push_back(raw);

    if(match("{"))
    {
        auto body =
            makeNode(
                ASTNodeType::CompoundStatement,
                "",
                current().line);

        parseCompound(body.get());
        node->addChild(std::move(body));
    }

    rootNode_->addChild(std::move(node));
}

void Parser::parseVariable(const std::string& name)
{
    auto node =
        makeNode(
            ASTNodeType::Variable,
            name,
            current().line);

    globalVariableList_.push_back(node.get());
    rootNode_->addChild(std::move(node));

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
    if(compound == nullptr)
    {
        return;
    }

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

        std::unique_ptr<ASTNode> stmt = parseStatement();

        if(stmt)
        {
            compound->addChild(std::move(stmt));
        }
    }

    if(match("}"))
    {
        advance();
    }
    else
    {
        setError("Unterminated compound statement");
    }
}

std::unique_ptr<ASTNode> Parser::parseStatement()
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

std::unique_ptr<ASTNode> Parser::parseIfStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    auto node =
        makeNode(
            ASTNodeType::IfStatement,
            cond,
            line);

    node->addChild(
        makeNode(
            ASTNodeType::Expression,
            cond,
            line));

    auto thenBranch = parseStatement();

    if(thenBranch)
    {
        node->addChild(std::move(thenBranch));
    }

    if(match("else"))
    {
        advance();

        auto elseBranch = parseStatement();

        if(elseBranch)
        {
            node->addChild(std::move(elseBranch));
        }
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseWhileStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    auto node =
        makeNode(
            ASTNodeType::WhileStatement,
            cond,
            line);

    node->addChild(
        makeNode(
            ASTNodeType::Expression,
            cond,
            line));

    auto body = parseStatement();

    if(body)
    {
        node->addChild(std::move(body));
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseForStatement()
{
    int line = current().line;
    advance();

    std::string header;

    if(match("("))
    {
        header = collectBalanced('(', ')');
    }

    auto node =
        makeNode(
            ASTNodeType::ForStatement,
            header,
            line);

    node->addChild(
        makeNode(
            ASTNodeType::Expression,
            header,
            line));

    auto body = parseStatement();

    if(body)
    {
        node->addChild(std::move(body));
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseDoWhileStatement()
{
    int line = current().line;
    advance();

    auto node =
        makeNode(
            ASTNodeType::DoWhileStatement,
            "",
            line);

    auto body = parseStatement();

    if(body)
    {
        node->addChild(std::move(body));
    }

    if(match("while"))
    {
        advance();

        std::string cond;

        if(match("("))
        {
            cond = collectBalanced('(', ')');
        }

        node->addChild(
            makeNode(
                ASTNodeType::Expression,
                cond,
                current().line));
    }

    if(match(";"))
    {
        advance();
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseSwitchStatement()
{
    int line = current().line;
    advance();

    std::string cond;

    if(match("("))
    {
        cond = collectBalanced('(', ')');
    }

    auto node =
        makeNode(
            ASTNodeType::SwitchStatement,
            cond,
            line);

    node->addChild(
        makeNode(
            ASTNodeType::Expression,
            cond,
            line));

    auto body = parseCompoundStatement();

    if(body)
    {
        node->addChild(std::move(body));
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseBreakStatement()
{
    int line = current().line;
    advance();

    if(match(";"))
    {
        advance();
    }

    return makeNode(
        ASTNodeType::BreakStatement,
        "break",
        line);
}

std::unique_ptr<ASTNode> Parser::parseContinueStatement()
{
    int line = current().line;
    advance();

    if(match(";"))
    {
        advance();
    }

    return makeNode(
        ASTNodeType::ContinueStatement,
        "continue",
        line);
}

std::unique_ptr<ASTNode> Parser::parseReturnStatement()
{
    int line = current().line;
    advance();

    std::string expr =
        collectUntilSemicolon();

    if(match(";"))
    {
        advance();
    }

    return makeNode(
        ASTNodeType::ReturnStatement,
        expr,
        line);
}

std::unique_ptr<ASTNode> Parser::parseCompoundStatement()
{
    auto compound =
        makeNode(
            ASTNodeType::CompoundStatement,
            "",
            current().line);

    parseCompound(compound.get());
    return compound;
}

std::unique_ptr<ASTNode> Parser::parseLocalDeclaration()
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

    auto node =
        makeNode(
            ASTNodeType::Variable,
            name,
            line);

    if(match("="))
    {
        advance();

        std::string init =
            collectUntilSemicolon();

        node->addChild(
            makeNode(
                ASTNodeType::Expression,
                init,
                line));
    }

    if(match(";"))
    {
        advance();
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseExpressionStatement()
{
    int line = current().line;

    std::string text =
        collectUntilSemicolon();

    if(match(";"))
    {
        advance();
    }

    return makeNode(
        ASTNodeType::Expression,
        text,
        line);
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
    bool closed = false;

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
                closed = true;
                break;
            }
        }

        result += text;
        advance();
    }

    if(!closed)
    {
        setError(
            std::string("Unmatched '") +
            open +
            "' starting at line " +
            std::to_string(current().line));
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
