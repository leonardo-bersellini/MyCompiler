#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <QString>

#include "token.h"
#include "types.h"

/** SYMBOL
 *  per simbolo si intendono tutti gli identifier ed in generale le parole che fungono da
 *  riferimento per qualcosa (variabili per valore, funzioni per parti di codice), e che non sono
 *  parole chiave, ma scritte in modo arbitrario dall'utente.
 **/

struct SymbolInfo {
    ValueType type;
};

/** FUNCTION INFO
 *  Questa struttura permette di riassumere i dati delle funzioni, permettendo le analisi semantiche
 *  in base alla tipologia dei dati.
 */

struct FunctionInfo {
    ValueType returnType;
    std::vector<ValueType> paramTypes;
};

#endif // SYMBOLS_H
