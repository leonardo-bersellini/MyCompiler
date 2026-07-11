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
    IfKeyword,
    ElseKeyword,
    ElifKeyword,
    ForKeyword,
    WhileKeyword,
    ReturnKeyword,
    VoidKeyword,

    BreakKeyword,
    ContinueKeyword,

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
    Comma,
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
    case TokenType::BoolLiteral: typeStr = "BoolLiteral";
        break;
    case TokenType::LogicalAnd: typeStr = "AND";
        break;
    case TokenType::LogicalOr: typeStr = "OR";
        break;
    case TokenType::IfKeyword: typeStr = "IfKeyword";
        break;
    case TokenType::ElseKeyword: typeStr = "ElseKeyword";
        break;
    case TokenType::ElifKeyword: typeStr = "ElifKeyword";
        break;
    case TokenType::ForKeyword: typeStr = "ForKeyword";
        break;
    case TokenType::BreakKeyword: typeStr = "BreakKeyword";
        break;
    case TokenType::ContinueKeyword: typeStr = "ContinueKeyword";
        break;
    case TokenType::WhileKeyword: typeStr = "WhileKeyword";
        break;
    case TokenType::ReturnKeyword: typeStr = "ReturnKeyword";
        break;
    case TokenType::VoidKeyword: typeStr = "VoidKeyword";
        break;
    case TokenType::EqualEqual: typeStr = "EqualEqual";
        break;
    case TokenType::NotEqual: typeStr = "NotEqual";
        break;
    case TokenType::Less: typeStr = "Less";
        break;
    case TokenType::LessEqual: typeStr = "LessEqual";
        break;
    case TokenType::Greater: typeStr = "Greater";
        break;
    case TokenType::GreaterEqual: typeStr = "GreaterEqual";
        break;
    case TokenType::LogicalNot: typeStr = "LogicalNot";
        break;
    case TokenType::LBracket: typeStr = "LBracket";
        break;
    case TokenType::RBracket: typeStr = "RBracket";
        break;
    case TokenType::LBrace: typeStr = "LBrace";
        break;
    case TokenType::RBrace: typeStr = "RBrace";
        break;
    case TokenType::Comma: typeStr = "Comma";
        break;

    Q_UNREACHABLE();
    }
    return typeStr;
}


#endif // TOKEN_H
