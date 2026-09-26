#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <unordered_map>
#include <string>
#include "token.h"

inline const std::unordered_map<std::string, TokenType> keywords = {
    {"int", TokenType::TypeKeyword},
    {"double", TokenType::TypeKeyword},
    {"char", TokenType::TypeKeyword},
    {"bool", TokenType::TypeKeyword},
    {"true", TokenType::BoolLiteral},
    {"false", TokenType::BoolLiteral},
    {"if", TokenType::IfKeyword},
    {"else", TokenType::ElseKeyword},
    {"elif", TokenType::ElifKeyword},
    {"for", TokenType::ForKeyword},
    {"while", TokenType::WhileKeyword},
    {"break", TokenType::BreakKeyword}, 
    {"continue", TokenType::ContinueKeyword},
    {"void", TokenType::VoidKeyword},
    {"return", TokenType::ReturnKeyword},
    {"switch", TokenType::SwitchKeyword},
    {"case", TokenType::CaseKeyword},
    {"default", TokenType::DefaultKeyword},
    {"const", TokenType::ConstKeyword},
    {"namespace", TokenType::NamespaceKeyword},
    {"print", TokenType::PrintKeyword},
};

#endif // KEYWORDS_H
