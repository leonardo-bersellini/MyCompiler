#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <string>
#include <vector>
#include <variant>

#include "token.h"
#include "types.h"

#include "utils/visitor/template_visitor.h"


struct VariableSymbol {
public:
    explicit VariableSymbol(const Type& t, const bool& c) : type(t), isConst(c) {}
    
    Type type;
    bool isConst;
};

struct FunctionSymbol {
public:
    explicit FunctionSymbol(const Type& rty, const std::vector<Type>& prm) 
        : returnType(rty), paramTypes(prm) {}
        
    Type returnType;
    std::vector<Type> paramTypes;
};

/** SYMBOL
 *  per simbolo si intendono tutti gli identifier ed in generale le parole che fungono da
 *  riferimento per qualcosa (variabili per valore, funzioni per parti di codice), e che non sono
 *  parole chiave, ma scritte in modo arbitrario dall'utente.
 * 
 *  La struttura symbol può assumere diverse forme di symbol, poichè esistono più tipologie 
 *  di simboli con membri e informazioni diverse.
 **/

template<class... Ts>
using SymbolVisitor = overloaded<Ts...>;

using SymbolCategory = std::variant<VariableSymbol, FunctionSymbol>;

struct Symbol 
{
    SymbolCategory category;
};

#endif // SYMBOLS_H
