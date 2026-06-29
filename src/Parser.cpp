#include "Parser.h"
#include <iostream>

namespace ModernPDE
{

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
        rootNode_ = nullptr;
    }
    
    rootNode_ = new ASTNode(ASTNodeType::TranslationUnit, "TranslationUnit");
    
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
    // حداقل سه توکن لازم داریم
    if(index_ + 2 >= tokenList_.size())
    {
        advance();
        return;
    }
    
    //--------------------------------------------------
    // type
    //--------------------------------------------------
    
    if(current().type != TokenType::Keyword)
    {
        advance();
        return;
    }
    
    std::string typeName = current().text;
    advance();
    
    //--------------------------------------------------
    // identifier
    //--------------------------------------------------
    
    if(!match(TokenType::Identifier))
    {
        return;
    }
    
    std::string name = current().text;
    advance();
    
    //--------------------------------------------------
    // Function ?
    //--------------------------------------------------
    
    if(match("("))
    {
        parseFunction(name);
        return;
    }
    
    //--------------------------------------------------
    // Variable ?
    //--------------------------------------------------
    
    parseVariable(name);
}

void Parser::parseFunction(const std::string& name)
{
    ASTNode* node = new ASTNode(ASTNodeType::Function, name);
    functionList_.push_back(node);
    rootNode_->children.push_back(node);
    
    //--------------------------------------------------
    // Skip parameter list
    //--------------------------------------------------
    
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
    
    //--------------------------------------------------
    // Parse body
    //--------------------------------------------------
    
    if(match("{"))
    {
        parseCompound();
    }
}

void Parser::parseVariable(const std::string& name)
{
    ASTNode* node = new ASTNode(ASTNodeType::Variable, name);
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

void Parser::parseCompound()
{
    if(match("{"))
    {
        advance();
    }
    
    int depth = 1;
    
    while(!eof() && depth > 0)
    {
        if(match("{"))
        {
            depth++;
        }
        else if(match("}"))
        {
            depth--;
            
            if(depth == 0)
            {
                advance();
                break;
            }
        }
        else if(match("if"))
        {
            parseIf();
            continue;
        }
        else if(match("while"))
        {
            parseWhile();
            continue;
        }
        else if(match("for"))
        {
            parseFor();
            continue;
        }
        else if(match("return"))
        {
            parseReturn();
            continue;
        }
        
        advance();
    }
}

void Parser::parseIf()
{
    // اینجا می‌توانید پیاده‌سازی کنید
    advance(); // skip 'if'
    // پرش تا } 
    int depth = 0;
    while(!eof())
    {
        if(match("{"))
        {
            depth++;
        }
        else if(match("}"))
        {
            if(depth == 0)
            {
                advance();
                break;
            }
            depth--;
        }
        advance();
    }
}

void Parser::parseWhile()
{
    // اینجا می‌توانید پیاده‌سازی کنید
    advance(); // skip 'while'
    int depth = 0;
    while(!eof())
    {
        if(match("{"))
        {
            depth++;
        }
        else if(match("}"))
        {
            if(depth == 0)
            {
                advance();
                break;
            }
            depth--;
        }
        advance();
    }
}

void Parser::parseFor()
{
    // اینجا می‌توانید پیاده‌سازی کنید
    advance(); // skip 'for'
    int depth = 0;
    while(!eof())
    {
        if(match("{"))
        {
            depth++;
        }
        else if(match("}"))
        {
            if(depth == 0)
            {
                advance();
                break;
            }
            depth--;
        }
        advance();
    }
}

void Parser::parseReturn()
{
    // اینجا می‌توانید پیاده‌سازی کنید
    advance(); // skip 'return'
    while(!eof() && !match(";"))
    {
        advance();
    }
    if(match(";"))
    {
        advance();
    }
}

} // namespace ModernPDE