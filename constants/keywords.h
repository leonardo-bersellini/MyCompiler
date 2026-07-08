#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <QMap>
#include <QString>
#include "token.h"

inline const QMap<QString, TokenType> keywords = {
    {"int", TokenType::TypeKeyword},
    {"double", TokenType::TypeKeyword},
    {"char", TokenType::TypeKeyword},
    // {"string", TokenType::TypeKeyword}, // disabilitato nel codegen
    {"bool", TokenType::TypeKeyword},
    {"true", TokenType::BoolLiteral},
    {"false", TokenType::BoolLiteral},
    {"if", TokenType::IfKeyword},
    {"else", TokenType::ElseKeyword},
    {"elif", TokenType::ElifKeyword},
    {"for", TokenType::ForKeyword},
    {"while", TokenType::WhileKeyword},
    // {"break", TokenType::BreakKeyword}, // disabilitato nel codegen
    // {"continue", TokenType::ContinueKeyword}, // disabilitato nel codegen
    {"void", TokenType::VoidKeyword},
    {"return", TokenType::ReturnKeyword},
};

#endif // KEYWORDS_H
