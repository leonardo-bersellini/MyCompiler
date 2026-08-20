#ifndef TYPES_H
#define TYPES_H

#include <string>
#include "token.h"

/**
 *  TYPES
 *  Questo header contiene tutta la logica di gestione e creazione dei tipi delle variabili.
 *  La presenza di tutta la logica di promozione e conversione in questo documento permette
 *  l'esistenza di una sola "souce of thruth" nell logica dei tipi condivisa tra analizzatore
 *  semantico e code generator.
 */

enum class ValueType {
    Int,
    Double,
    String,
    Char,
    Bool,
    Void,
    Error
};

namespace Type
{
    inline bool isNumeric(ValueType t);
    inline bool isTextual(ValueType t);
    inline bool isAssignmentCompatible(ValueType destination, ValueType source);
    inline bool isEqualityComparable(ValueType left, ValueType right);
    inline bool isRelationalComparable(ValueType left, ValueType right);
    inline ValueType arithmeticResultType(ValueType left, ValueType right);
    inline ValueType additionResultType(ValueType left, ValueType right);
    inline ValueType equalityResultType(ValueType left, ValueType right);
    inline ValueType relationalResultType(ValueType left, ValueType right);
    inline ValueType logicalResultType(ValueType left, ValueType right);
    inline ValueType binaryResultType(TokenType _operator, ValueType left, ValueType right);
    inline ValueType unaryResultType(TokenType _operator, ValueType left);
    inline ValueType promotionType(ValueType first, ValueType second);
    inline std::string toString(const ValueType& type);
    inline ValueType toValueType(const std::string& typeName);

    // TODO aggiungere commenti alla sezione Type
}

/*
 * Ritorna true se il valore è considerato numerico
 */

bool Type::isNumeric(ValueType t) {
    return t == ValueType::Int || t == ValueType::Double;
}

/*
 * Ritorna true se il valore è considerato testuale
 */

bool Type::isTextual(ValueType t) {
    return t == ValueType::Char || t == ValueType::String;
}

/*
 * Questa funzione controlla se un'operazione di assegnazione è fattibile in base ai tipi degli
 * operandi.
 * I due parametri rappresentano il tipo della variabile a cui si assegna il valore (variableType) ed
 * il tipo del valore che si sta assegnando (assignType).
 * es: int x = 5.0; (variableType è int mentre assignType è double).
 */

bool Type::isAssignmentCompatible(ValueType destination, ValueType source) {
    if (destination == ValueType::Error) return true;  // errore già segnalato altrove, non duplicare
    if (source == ValueType::Error) return true;

    if (destination == source) return true;

    // promozione Int -> Double
    if (destination == ValueType::Double && source == ValueType::Int) return true;

    return false;
}

/*
 * Questa funzione permette di stabilire se due tipi possono essere confrontati
 * tra loro.
  */

bool Type::isEqualityComparable(ValueType left, ValueType right)
{
    if(left == right)
        return true;

    if (isNumeric(left) && isNumeric(right))
        return true;

    return false;
}

bool Type::isRelationalComparable(ValueType left, ValueType right)
{
    if(left == right)
        return true;

    if(isNumeric(left) && isNumeric(right))
        return true;

    return false;
}

ValueType Type::arithmeticResultType(ValueType left, ValueType right)
{
    if (!isNumeric(left) || !isNumeric(right))
        return ValueType::Error;

    return (left == ValueType::Double || right == ValueType::Double)
               ? ValueType::Double
               : ValueType::Int;
}

ValueType Type::additionResultType(ValueType left, ValueType right)
{
    // caso numerico
    if (isNumeric(left) && isNumeric(right))
    {
        return (left == ValueType::Double || right == ValueType::Double)
                   ? ValueType::Double
                   : ValueType::Int;
    }

    // Caso testuale
    bool leftTextual = isTextual(left);
    bool rightTextual = isTextual(right);

    if(leftTextual || rightTextual)
    {
        // 'a' + 'b' = "ab";
        if(left == ValueType::Char && right == ValueType::Char)
            return ValueType::String;

        // Almeno uno dei due è una stringa -> concatenazione
        if((left == ValueType::String && rightTextual) || (right == ValueType::String && leftTextual))
            return ValueType::String;
    }

    return ValueType::Error;
}

ValueType Type::equalityResultType(ValueType left, ValueType right)
{
    return isEqualityComparable(left, right)
        ? ValueType::Bool
        : ValueType::Error;
}

ValueType Type::relationalResultType(ValueType left, ValueType right)
{
    return isRelationalComparable(left, right)
        ? ValueType::Bool
        : ValueType::Error;
}

ValueType Type::logicalResultType(ValueType left, ValueType right)
{
    return (left == ValueType::Bool && right == ValueType::Bool)
           ? ValueType::Bool
           : ValueType::Error;
}

ValueType Type::binaryResultType(TokenType _operator, ValueType left, ValueType right)
{
    if (left == ValueType::Error || right == ValueType::Error)
        return ValueType::Error;

    /* Controllo dei tipi di ritorno */

    switch(_operator)
    {
        // Uguaglianza / Disuguaglianza : operatori [ ==, != ]
        case TokenType::EqualEqual:
        case TokenType::NotEqual  :

        return equalityResultType(left, right);

        // Confronti Relazionali : operatori [ >, <, >=, <= ]
        case TokenType::Less :
        case TokenType::Greater :
        case TokenType::LessEqual :
        case TokenType::GreaterEqual :

            return relationalResultType(left, right);

        // Operatori Logici : operatori [ &&, || ]
        case TokenType::LogicalAnd :
        case TokenType::LogicalOr  :

            return logicalResultType(left, right);

        // Somma Aritmetica : operatore [ + ]
        case TokenType::Plus :

            return additionResultType(left, right);

        // Minus, Star, Slash : operatori [ -, *, / ]
        case TokenType::Minus :
        case TokenType::Slash :
        case TokenType::Star :

            return arithmeticResultType(left, right);

        default: return ValueType::Error;
            break;
    }

    return ValueType::Error;
}

ValueType Type::unaryResultType(TokenType _operator, ValueType left)
{
    if(left == ValueType::Error)
        return ValueType::Error;

    switch(_operator) {
        // Operatori logici : operatori [!]
        case TokenType::LogicalNot:
            if (left == ValueType::Bool) {
                return ValueType::Bool;
            }
            return ValueType::Error;

        // Operatori aritmetici : operatori [+, -]
        case TokenType::Minus:
        case TokenType::Plus:
            if (isNumeric(left)) {
                return left; // Il tipo rimane lo stesso (Int resta Int, Double resta Double)
            }
            return ValueType::Error;

        default:
            return ValueType::Error;
    }
}

ValueType Type::promotionType(ValueType first, ValueType second)
{
    if (first == ValueType::Error || second == ValueType::Error)
        return ValueType::Error;

    if (first == second)
        return first;

    // int + double = double
    if (isNumeric(first) && isNumeric(second))
        return ValueType::Double;

    return ValueType::Error;
}

std::string Type::toString(const ValueType& type) {
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
    case ValueType::Void: return "Void";
        break;
    }
    return "Unknown";
}

ValueType Type::toValueType(const std::string& typeName) {
    if (typeName == "int") return ValueType::Int;
    if (typeName == "double") return ValueType::Double;
    if (typeName == "string") return ValueType::String;
    if (typeName == "char") return ValueType::Char;
    if (typeName == "bool") return ValueType::Bool;
    if( typeName == "void") return ValueType::Void;
    return ValueType::Error;
}

#endif // TYPES_H
