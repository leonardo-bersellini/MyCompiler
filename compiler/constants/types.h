#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <optional>
#include <stdexcept>
#include <variant>

#include "token.h"

#include "utils/visitor/template_visitor.h"

template<class... Ts>
using TypeVisitor = overloaded<Ts...>;

/**
 *  TYPES
 *  Questo header contiene tutta la logica di gestione e creazione dei tipi delle variabili.
 *  La presenza di tutta la logica di promozione e conversione in questo documento permette
 *  l'esistenza di una sola "souce of thruth" nell logica dei tipi condivisa tra analizzatore
 *  semantico e code generator.
 */

enum class PrimitiveType {
    Int,   
    Double, 
    Char,
    Bool,
    String,
    Void,
    Error
};


struct ArrayType 
{
    ArrayType() : elementType(PrimitiveType::Void), size(0) {}
    ArrayType(const PrimitiveType t, const std::size_t s) : elementType(t), size(s) {}
    PrimitiveType elementType;
    std::size_t size;

    bool operator==(const ArrayType& other) const {
        if(elementType != other.elementType) return false;
        return size == other.size;
    }

};

/*
 * Type
 * Type rappresenta il tipo di una variabile, che può essere primitivo o di un tipo più 
 * strutturato (come arraytype).
 * Per questo type non fa altro che contenere una variante che può assumere un valore possibile
 * del tipo di una variabile.
 */

// variante: possibili tipi assunti da Type
using TypeCategory = std::variant<PrimitiveType, ArrayType>;

struct Type {
public:
    TypeCategory category;

    bool is(PrimitiveType p) const {
        return std::holds_alternative<PrimitiveType>(category) && std::get<PrimitiveType>(category) == p;
    }

    bool isArray() const {
        return std::holds_alternative<ArrayType>(category);
    }

    bool isError() const {
        return std::visit(TypeVisitor{
            [](const PrimitiveType& p) { return p == PrimitiveType::Error; },
            [](const ArrayType& a) { return a.elementType == PrimitiveType::Error; }
        }, category);
    }

    PrimitiveType asPrimitive() const {
        if (std::holds_alternative<PrimitiveType>(category)) return std::get<PrimitiveType>(category);
        return PrimitiveType::Error;
    }
};


namespace types
{
    inline bool isNumeric(PrimitiveType t);
    inline bool isTextual(PrimitiveType t);
    inline bool isAssignmentCompatible(Type destination, Type source);
    inline bool isEqualityComparable(PrimitiveType left, PrimitiveType right);
    inline bool isRelationalComparable(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType arithmeticResultType(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType additionResultType(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType equalityResultType(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType relationalResultType(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType logicalResultType(PrimitiveType left, PrimitiveType right);
    inline PrimitiveType binaryResultType(TokenType _operator, PrimitiveType left, PrimitiveType right);
    inline PrimitiveType unaryResultType(TokenType _operator, PrimitiveType left);
    inline PrimitiveType promotionType(PrimitiveType first, PrimitiveType second);
    inline std::string toString(const Type& type);
    inline PrimitiveType toPrimitiveType(const std::string& typeName);

    // TODO aggiungere commenti alla sezione Type
}

/*
 * Ritorna true se il valore è considerato numerico
 */

inline bool types::isNumeric(PrimitiveType t) {
    return t == PrimitiveType::Int || t == PrimitiveType::Double;
}

/*
 * Ritorna true se il valore è considerato testuale
 */

inline bool types::isTextual(PrimitiveType t) {
    return t == PrimitiveType::Char || t == PrimitiveType::String;
}

/*
 * Questa funzione controlla se un'operazione di assegnazione è fattibile in base ai tipi degli
 * operandi.
 * I due parametri rappresentano il tipo della variabile a cui si assegna il valore (variableType) ed
 * il tipo del valore che si sta assegnando (assignType).
 * es: int x = 5.0; (variableType è int mentre assignType è double).
 */

inline bool types::isAssignmentCompatible(Type destination, Type source)
{
    return std::visit(TypeVisitor{
        [](const PrimitiveType& d, const PrimitiveType& s) -> bool {
            if(d == PrimitiveType::Error || s == PrimitiveType::Error) return true; // errore già segnalato altrove, non duplicare
            if(d == s) return true;
            // promozione Int -> Double
            if (d == PrimitiveType::Double && s == PrimitiveType::Int) return true;
            return false;
        },
        [](const PrimitiveType& d, const ArrayType& s) -> bool {
            return false;
        },
        [](const ArrayType& d, const PrimitiveType s) -> bool {
            return false;
        },
        [](const ArrayType& d, const ArrayType& s) -> bool {
            if(d.elementType == PrimitiveType::Error || s.elementType == PrimitiveType::Error) return false;
            if(d.elementType != s.elementType) return false;
            return d.size == s.size;
        },

    }, destination.category, source.category);
}

/*
 * Questa funzione permette di stabilire se due tipi possono essere confrontati
 * tra loro.
 */

inline bool types::isEqualityComparable(PrimitiveType left, PrimitiveType right)
{
    if(left == right)
        return true;

    if (isNumeric(left) && isNumeric(right))
        return true;

    return false;
}

inline bool types::isRelationalComparable(PrimitiveType left, PrimitiveType right)
{
    if(left == right)
        return true;

    if(isNumeric(left) && isNumeric(right))
        return true;

    return false;
}

inline PrimitiveType types::arithmeticResultType(PrimitiveType left, PrimitiveType right)
{
    if (!isNumeric(left) || !isNumeric(right))
        return PrimitiveType::Error;

    return (left == PrimitiveType::Double || right == PrimitiveType::Double)
               ? PrimitiveType::Double
               : PrimitiveType::Int;
}

inline PrimitiveType types::additionResultType(PrimitiveType left, PrimitiveType right)
{
    // caso numerico
    if (isNumeric(left) && isNumeric(right))
    {
        return (left == PrimitiveType::Double || right == PrimitiveType::Double)
                   ? PrimitiveType::Double
                   : PrimitiveType::Int;
    }

    // Caso testuale
    bool leftTextual = isTextual(left);
    bool rightTextual = isTextual(right);

    if(leftTextual || rightTextual)
    {
        // 'a' + 'b' = "ab";
        if(left == PrimitiveType::Char && right == PrimitiveType::Char)
            return PrimitiveType::String;

        // Almeno uno dei due è una stringa -> concatenazione
        if((left == PrimitiveType::String && rightTextual) || (right == PrimitiveType::String && leftTextual))
            return PrimitiveType::String;
    }

    return PrimitiveType::Error;
}

inline PrimitiveType types::equalityResultType(PrimitiveType left, PrimitiveType right)
{
    return isEqualityComparable(left, right)
        ? PrimitiveType::Bool
        : PrimitiveType::Error;
}

inline PrimitiveType types::relationalResultType(PrimitiveType left, PrimitiveType right)
{
    return isRelationalComparable(left, right)
        ? PrimitiveType::Bool
        : PrimitiveType::Error;
}

inline PrimitiveType types::logicalResultType(PrimitiveType left, PrimitiveType right)
{
    return (left == PrimitiveType::Bool && right == PrimitiveType::Bool)
           ? PrimitiveType::Bool
           : PrimitiveType::Error;
}

inline PrimitiveType types::binaryResultType(TokenType _operator, PrimitiveType left, PrimitiveType right)
{
    if (left == PrimitiveType::Error || right == PrimitiveType::Error)
        return PrimitiveType::Error;

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

        default: return PrimitiveType::Error;
            break;
    }

    return PrimitiveType::Error;
}

inline PrimitiveType types::unaryResultType(TokenType _operator, PrimitiveType left)
{
    if(left == PrimitiveType::Error)
        return PrimitiveType::Error;

    switch(_operator) {
        // Operatori logici : operatori [!]
        case TokenType::LogicalNot:
            if (left == PrimitiveType::Bool) {
                return PrimitiveType::Bool;
            }
            return PrimitiveType::Error;

        // Operatori aritmetici : operatori [+, -]
        case TokenType::Minus:
        case TokenType::Plus:
            if (isNumeric(left)) {
                return left; // Il tipo rimane lo stesso (Int resta Int, Double resta Double)
            }
            return PrimitiveType::Error;

        default:
            return PrimitiveType::Error;
    }
}

inline PrimitiveType types::promotionType(PrimitiveType first, PrimitiveType second)
{
    if (first == PrimitiveType::Error || second == PrimitiveType::Error)
        return PrimitiveType::Error;

    if (first == second)
        return first;

    // int + double = double
    if (isNumeric(first) && isNumeric(second))
        return PrimitiveType::Double;

    return PrimitiveType::Error;
}

inline std::string types::toString(const Type& type) {
    return std::visit(TypeVisitor{
        [](const PrimitiveType& t) -> std::string {
            switch(t) {
                case PrimitiveType::Int:    return "int";
                case PrimitiveType::Double: return "double";
                case PrimitiveType::Char:   return "char";
                case PrimitiveType::Bool:   return "bool";
                case PrimitiveType::String:  return "string";
                case PrimitiveType::Void:    return "void";
                case PrimitiveType::Error:   return "error";
            }
            return "Unknown";
        },
        [](const ArrayType& a) -> std::string {
            std::string base_type = toString(Type{a.elementType});
            return base_type + "[" + std::to_string(a.size) + "]";
        },

    }, type.category);
}

inline PrimitiveType types::toPrimitiveType(const std::string& typeName) {
    if (typeName == "int")    return PrimitiveType::Int;
    if (typeName == "double") return PrimitiveType::Double;
    if (typeName == "string") return PrimitiveType::String;
    if (typeName == "char")   return PrimitiveType::Char;
    if (typeName == "bool")   return PrimitiveType::Bool;
    if( typeName == "void")   return PrimitiveType::Void;
    return PrimitiveType::Error;
}

#endif // TYPES_H
