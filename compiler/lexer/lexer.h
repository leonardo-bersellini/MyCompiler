#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "token.h"
#include "errorlog/errorlog.h"

class Lexer
{
public:
    Lexer();

    std::vector<Token> analiseString(const std::string& string, ErrorLog& _errorLog);

    void printTokens();

private:
    std::vector<Token> m_tokens;
    std::string buffer;
    int indexPos;
    TextPosition currentTextPos;
    ErrorLog* errorLog;

    char peek(int offset = 0) const;  //guarda i caratteri futuri con un offset specificato > 0
    char advance();                   //mangia il carattere seguente rispetto all'indica del lexer

    bool isAtEnd() const;
    bool isAtEnd(int pos) const;

    bool isDigit(const char& c) const;
    bool isAlpha(const char& c) const;

    Token createToken(TokenType type);

    Token scanNumber();
    Token scanIdentifier();
    Token scanString();
    Token scanChar();

    std::string removeAll(std::string str, const std::string& sub);

    std::string tokenToString(Token t);

};

#endif // LEXER_H
