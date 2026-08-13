#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace ModernPDE
{

enum class TokenType
{
    Unknown,

    Identifier,
    Keyword,

    Integer,
    Float,
    String,
    Character,

    Operator,
    Delimiter,

    Comment,

    EndOfFile
};

struct Token
{
    TokenType type;

    std::string text;

    int line;
    int column;
};

class Lexer
{
public:

    Lexer();

    bool tokenizeFile(const std::string& filename);

    bool tokenize(const std::string& source);

    const std::vector<Token>& tokens() const;

    bool hasError() const;

    const std::string& lastError() const;

private:

    std::string source_;

    std::vector<Token> tokenList_;

    size_t pos_;

    int line_;

    int column_;

private:

    void clear();

    bool eof() const;

    char current() const;

    char peek(int offset = 1) const;

    char advance();

    void skipWhitespace();

    void scanIdentifier();

    void scanNumber();

    void scanString();

    void scanCharacter();

    void scanComment();

    void scanOperator();

    void scanDelimiter();

    void addToken(
        TokenType type,
        const std::string& text,
        int line,
        int column);

    bool isIdentifierStart(char c) const;

    bool isIdentifierBody(char c) const;

    bool isKeyword(const std::string& word) const;

private:

    std::unordered_set<std::string> keywords_;

    std::string lastError_;
};

}

