#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <QMap>
#include <QString>
#include "token.h"

inline const QMap<QString, TokenType> keywords = {
    {"int", TokenType::TypeKeyword},
    {"double", TokenType::TypeKeyword},
    {"char", TokenType::TypeKeyword},
    {"string", TokenType::TypeKeyword},
    {"bool", TokenType::TypeKeyword},
    {"true", TokenType::BoolLiteral},
    {"false", TokenType::BoolLiteral},
    // futuro: {"if", TokenType::If}, {"while", TokenType::While}
};

#endif // KEYWORDS_H
