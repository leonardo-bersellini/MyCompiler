#ifndef SYMBOL_H
#define SYMBOL_H

#include <QString>

#include "token.h"

enum class ValueType {
    Int,
    Double,
    String,
    Char,
    Bool,
    Error
};

inline QString toString(const ValueType& type) {
    switch(type) {
        case ValueType::Int: return "Int";
            break;
        case ValueType::Double: return "Double";
            break;
        case ValueType::String: return "String";
            break;
        case ValueType::Char: return "Char";
            break;
        case ValueType::Bool: return "Bool";
            break;
        case ValueType::Error: return "Error";
            break;
    }
    return "Unknown";
}

inline ValueType toValueType(const QString& typeName) {
    if (typeName == "int") return ValueType::Int;
    if (typeName == "double") return ValueType::Double;
    if (typeName == "string") return ValueType::String;
    if (typeName == "char") return ValueType::Char;
    if (typeName == "bool") return ValueType::Bool;
    return ValueType::Error;
}

struct SymbolInfo {
    ValueType type;
};

#endif // SYMBOL_H
