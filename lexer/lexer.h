#ifndef LEXER_H
#define LEXER_H

#include <QString>
#include <QList>

#include "token.h"
#include "errorlog.h"

class Lexer
{
public:
    Lexer();

    QList<Token> analiseString(const QString& string, ErrorLog& _errorLog);

    void printTokens();

private:
    QList<Token> m_tokens;
    QString buffer;
    int indexPos;
    TextPosition currentTextPos;
    ErrorLog* errorLog;

    QChar peek(int offset = 0) const;  //guarda i caratteri futuri con un offset specificato > 0
    QChar advance();                   //mangia il carattere seguente rispetto all'indica del lexer

    bool isAtEnd() const;
    bool isAtEnd(int pos) const;

    Token createToken(TokenType type);

    Token scanNumber();
    Token scanIdentifier();
    Token scanString();
    Token scanChar();

    QString tokenToString(Token t);

};

#endif // LEXER_H
