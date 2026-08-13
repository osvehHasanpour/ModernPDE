#include "Lexer.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace ModernPDE
{

Lexer::Lexer()
{
    keywords_ =
    {
        "if",
        "else",
        "for",
        "while",
        "do",

        "switch",
        "case",
        "default",

        "break",
        "continue",
        "return",

        "class",
        "struct",
        "union",

        "public",
        "private",
        "protected",

        "virtual",
        "override",

        "namespace",

        "template",
        "typename",

        "const",
        "constexpr",
        "static",
        "inline",

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

        "auto",

        "new",
        "delete",

        "try",
        "catch",
        "throw",

        "using",

        "enum",

        "true",
        "false",

        "nullptr"
    };

    clear();
}

void Lexer::clear()
{
    tokenList_.clear();

    source_.clear();

    pos_ = 0;

    line_ = 1;

    column_ = 1;

    lastError_.clear();
}

bool Lexer::hasError() const
{
    return !lastError_.empty();
}

const std::string& Lexer::lastError() const
{
    return lastError_;
}

bool Lexer::tokenizeFile(const std::string& filename)
{
    clear();

    if(filename.empty())
    {
        lastError_ = "No input file specified";
        return false;
    }

    std::ifstream file(filename);

    if(!file)
    {
        lastError_ = "Cannot open file: " + filename;
        return false;
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    if(file.bad())
    {
        lastError_ = "Failed to read file: " + filename;
        return false;
    }

    return tokenize(buffer.str());
}

bool Lexer::tokenize(const std::string& source)
{
    clear();

    source_ = source;

    while(!eof())
    {
        skipWhitespace();

        if(eof())
            break;

        char c = current();

        if(isIdentifierStart(c))
        {
            scanIdentifier();
            continue;
        }

        if(std::isdigit((unsigned char)c))
        {
            scanNumber();
            continue;
        }

        if(c == '"')
        {
            scanString();
            continue;
        }

        if(c == '\'')
        {
            scanCharacter();
            continue;
        }

        if(c == '/')
        {
            if(peek() == '/' || peek() == '*')
            {
                scanComment();
                continue;
            }
        }

        if(std::ispunct((unsigned char)c))
        {
            scanOperator();
            continue;
        }

        advance();
    }

    addToken(
        TokenType::EndOfFile,
        "",
        line_,
        column_);

    return true;
}

const std::vector<Token>& Lexer::tokens() const
{
    return tokenList_;
}

bool Lexer::eof() const
{
    return pos_ >= source_.size();
}

char Lexer::current() const
{
    if(eof())
        return '\0';

    return source_[pos_];
}

char Lexer::peek(int offset) const
{
    size_t p = pos_ + offset;

    if(p >= source_.size())
        return '\0';

    return source_[p];
}

char Lexer::advance()
{
    char c = current();

    pos_++;

    if(c == '\n')
    {
        line_++;

        column_ = 1;
    }
    else
    {
        column_++;
    }

    return c;
}

void Lexer::skipWhitespace()
{
    while(!eof())
    {
        char c = current();

        if(c == ' ' ||
           c == '\t' ||
           c == '\r' ||
           c == '\n')
        {
            advance();
            continue;
        }

        break;
    }
}

bool Lexer::isIdentifierStart(char c) const
{
    return
        std::isalpha((unsigned char)c) ||
        c == '_';
}

bool Lexer::isIdentifierBody(char c) const
{
    return
        std::isalnum((unsigned char)c) ||
        c == '_';
}

bool Lexer::isKeyword(const std::string& word) const
{
    return keywords_.find(word) != keywords_.end();
}

void Lexer::scanIdentifier()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    while(!eof() &&
          isIdentifierBody(current()))
    {
        text += advance();
    }

    if(isKeyword(text))
    {
        addToken(
            TokenType::Keyword,
            text,
            startLine,
            startColumn);
    }
    else
    {
        addToken(
            TokenType::Identifier,
            text,
            startLine,
            startColumn);
    }
}

void Lexer::scanNumber()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    bool hasDot = false;

    while(!eof())
    {
        char c = current();

        if(std::isdigit((unsigned char)c))
        {
            text += advance();
            continue;
        }

        if(c == '.' && !hasDot)
        {
            hasDot = true;
            text += advance();
            continue;
        }

        break;
    }

    addToken(
        hasDot
            ? TokenType::Float
            : TokenType::Integer,
        text,
        startLine,
        startColumn);
}

void Lexer::scanString()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    text += advance();

    bool terminated = false;

    while(!eof())
    {
        char c = advance();

        text += c;

        if(c == '\\')
        {
            if(!eof())
                text += advance();

            continue;
        }

        if(c == '"')
        {
            terminated = true;
            break;
        }
    }

    if(!terminated && lastError_.empty())
    {
        lastError_ =
            "Unterminated string literal at line " +
            std::to_string(startLine);
    }

    addToken(
        TokenType::String,
        text,
        startLine,
        startColumn);
}

void Lexer::scanCharacter()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    text += advance();

    bool terminated = false;

    while(!eof())
    {
        char c = advance();

        text += c;

        if(c == '\\')
        {
            if(!eof())
                text += advance();

            continue;
        }

        if(c == '\'')
        {
            terminated = true;
            break;
        }
    }

    if(!terminated && lastError_.empty())
    {
        lastError_ =
            "Unterminated character literal at line " +
            std::to_string(startLine);
    }

    addToken(
        TokenType::Character,
        text,
        startLine,
        startColumn);
}

void Lexer::scanComment()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    // single line comment
    if(current() == '/' && peek() == '/')
    {
        text += advance();
        text += advance();

        while(!eof() && current() != '\n')
        {
            text += advance();
        }

        addToken(
            TokenType::Comment,
            text,
            startLine,
            startColumn);

        return;
    }

    // multi line comment
    if(current() == '/' && peek() == '*')
    {
        text += advance();
        text += advance();

        bool terminated = false;

        while(!eof())
        {
            char c = advance();

            text += c;

            if(c == '*' && current() == '/')
            {
                text += advance();
                terminated = true;
                break;
            }
        }

        if(!terminated && lastError_.empty())
        {
            lastError_ =
                "Unterminated block comment at line " +
                std::to_string(startLine);
        }

        addToken(
            TokenType::Comment,
            text,
            startLine,
            startColumn);

        return;
    }
}

void Lexer::scanOperator()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    char c = current();

    switch(c)
    {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '=':
        case '!':
        case '<':
        case '>':
        case '&':
        case '|':
        case '^':
        case '~':
        {
            text += advance();

            if(!eof())
            {
                char n = current();

                if((text[0] == '+' && (n == '+' || n == '=')) ||
                   (text[0] == '-' && (n == '-' || n == '=' || n == '>')) ||
                   (text[0] == '*' && n == '=') ||
                   (text[0] == '/' && n == '=') ||
                   (text[0] == '%' && n == '=') ||
                   (text[0] == '=' && n == '=') ||
                   (text[0] == '!' && n == '=') ||
                   (text[0] == '<' && (n == '=' || n == '<')) ||
                   (text[0] == '>' && (n == '=' || n == '>')) ||
                   (text[0] == '&' && (n == '&' || n == '=')) ||
                   (text[0] == '|' && (n == '|' || n == '=')) ||
                   (text[0] == '^' && n == '='))
                {
                    text += advance();
                }
            }

            addToken(
                TokenType::Operator,
                text,
                startLine,
                startColumn);

            return;
        }

        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case ';':
        case ':':
        case ',':
        case '.':
        case '?':
        {
            scanDelimiter();
            return;
        }

        default:
        {
            text += advance();

            addToken(
                TokenType::Unknown,
                text,
                startLine,
                startColumn);

            return;
        }
    }
}

void Lexer::scanDelimiter()
{
    int startLine = line_;
    int startColumn = column_;

    std::string text;

    text += advance();

    addToken(
        TokenType::Delimiter,
        text,
        startLine,
        startColumn);
}

void Lexer::addToken(
    TokenType type,
    const std::string& text,
    int line,
    int column)
{
    Token token;

    token.type = type;
    token.text = text;
    token.line = line;
    token.column = column;

    tokenList_.push_back(token);
}

} // namespace ModernPDE
