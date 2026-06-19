#ifndef TOKEN_H
#define TOKEN_H

#include <QString>

struct TextPosition {
    int line;
    int column;
};

enum class TokenType {
    IntegerLiteral,
    DoubleLiteral,
    StringLiteral,
    CharLiteral,
    BoolLiteral,

    Identifier,
    TypeKeyword,

    Plus,
    Minus,
    Star,
    Slash,

    Equal,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LogicalAnd,
    LogicalOr,
    LogicalNot,

    LParen,     //()
    RParen,
    LBracket,   //[]
    RBracket,
    LBrace,     //{}
    RBrace,

    Semicolon,
    EndOfFile,
    Unknown,
};

struct Token {
    TokenType type;
    double numericValue; //valore se il token corrisponde ad un numero
    QString lexeme;     //testo interpretato come token
    TextPosition position;
};

inline QString typeToString(TokenType type) {
    QString typeStr;
    switch(type) {
    case TokenType::IntegerLiteral: typeStr = "IntegerLiteral";
        break;
    case TokenType::DoubleLiteral:  typeStr = "DoubleLiteral";
        break;
    case TokenType::Identifier: typeStr = "Identifier";
        break;
    case TokenType::Plus:       typeStr = "Plus";
        break;
    case TokenType::Minus:      typeStr = "Minus";
        break;
    case TokenType::Star:       typeStr = "Star";
        break;
    case TokenType::Slash:      typeStr = "Slash";
        break;
    case TokenType::Equal:      typeStr = "Equal";
        break;
    case TokenType::LParen:     typeStr = "LParen";
        break;
    case TokenType::RParen:     typeStr = "RParen";
        break;
    case TokenType::Semicolon:  typeStr = "Semicolon";
        break;
    case TokenType::EndOfFile:  typeStr = "EndOfFile";
        break;
    case TokenType::Unknown:    typeStr = "Unknown";
        break;
    case TokenType::TypeKeyword: typeStr = "TypeKeyword";
        break;
    case TokenType::StringLiteral: typeStr = "StringLiteral";
        break;
    case TokenType::CharLiteral: typeStr = "CharLiteral";
        break;
    }
    return typeStr;
}


#endif // TOKEN_H
