#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <optional>
#include <stdexcept>
#include "token.h"

/**
 *  TYPES
 *  Questo header contiene tutta la logica di gestione e creazione dei tipi delle variabili.
 *  La presenza di tutta la logica di promozione e conversione in questo documento permette
 *  l'esistenza di una sola "souce of thruth" nell logica dei tipi condivisa tra analizzatore
 *  semantico e code generator.
 */

enum class PrimitiveType {
    Int,    ArrayInt,
    Double, ArrayDouble,
    Char,   ArrayChar,
    Bool,   ArrayBool,
    String,
    Void,
    Error
};


struct Type {
public:
    Type() = default;
    explicit Type(const PrimitiveType t) : primitive(t), size(std::nullopt) {}
    explicit Type(const PrimitiveType t, const std::size_t s) : primitive(t), size(s) {}
    
    PrimitiveType primitive;
    std::optional<std::size_t> size = std::nullopt; //per array

    bool operator==(const Type& other) const {
        if(primitive != other.primitive) return false;
        return size == other.size; // std::optional supporta == di default
    }
};


namespace types
{
    inline bool isNumeric(PrimitiveType t);
    inline bool isTextual(PrimitiveType t);
    inline bool isArray(PrimitiveType t);
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
    inline PrimitiveType toPrimitiveType(const std::string& typeName, bool isArray = false);

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
 *  Ritorna true se il tipo è riconducibile ad un array
 */

inline bool types::isArray(PrimitiveType t) {
    return (t == PrimitiveType::ArrayInt || t == PrimitiveType::ArrayDouble ||
            t == PrimitiveType::ArrayChar || t == PrimitiveType::ArrayBool);
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
    if (destination.primitive == PrimitiveType::Error) return true;  // errore già segnalato altrove, non duplicare
    if (source.primitive == PrimitiveType::Error) return true;

    if (destination == source) return true;

    // promozione Int -> Double
    if (destination.primitive == PrimitiveType::Double && source.primitive == PrimitiveType::Int) return true;

    return false;
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

/*
 * Converte un valore di PrimitiveType in stringa letterale.
 * Le stringhe ritornate da questa funzione devono essere complementari a quelle 
 * controllare nella funzione inversa (toPrimitiveType). In caso contrario, una conversione
 * prima a stringa e poi di nuovo a primitive risulterebbe in un errore.
 */

inline std::string types::toString(const Type& type) {
    switch(type.primitive) {
    case PrimitiveType::Int: return "int";
        break;
    case PrimitiveType::Double: return "double";
        break;
    case PrimitiveType::String: return "string";
        break;
    case PrimitiveType::Char: return "char";
        break;
    case PrimitiveType::Bool: return "bool";
        break;
    case PrimitiveType::Error: return "Error";
        break;
    case PrimitiveType::Void: return "void";
        break;
    }

    if(isArray(type.primitive)) 
    {
        std::string base_type;
        switch (type.primitive) {
            case PrimitiveType::ArrayInt:    base_type = "int";
                break;
            case PrimitiveType::ArrayDouble: base_type = "double";
                break;
            case PrimitiveType::ArrayChar:   base_type = "char";
                break;
            case PrimitiveType::ArrayBool:   base_type = "bool";
                break;
        
            default: throw std::runtime_error("internal error in types::tostring. control isArray failed");
        }

        std::string t = base_type + "[" + std::to_string(type.size.value()) + "]";
        return t;
    }

    return "Unknown";
}

inline PrimitiveType types::toPrimitiveType(const std::string& typeName, bool isArray) {
    if (typeName == "int")    return isArray ? PrimitiveType::ArrayInt : PrimitiveType::Int;
    if (typeName == "double") return isArray ? PrimitiveType::ArrayDouble : PrimitiveType::Double;
    if (typeName == "string") return PrimitiveType::String;
    if (typeName == "char")   return isArray ?  PrimitiveType::ArrayChar : PrimitiveType::Char;
    if (typeName == "bool")   return isArray ?  PrimitiveType::ArrayBool : PrimitiveType::Bool;
    if( typeName == "void")   return PrimitiveType::Void;
    return PrimitiveType::Error;
}

#endif // TYPES_H
