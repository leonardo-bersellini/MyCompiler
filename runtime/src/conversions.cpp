#ifndef _BSM_CONVERSIONS_
#define _BSM_CONVERSIONS_

#include <stdint.h>

// MODULO RUNTIME DI CONVERSIONE
//
// Questo modulo espone delle funzioni runtime di conversione dei tipi,
// utilizzabili da altre funzioni runtime.
//

// CONVERSIONE STRING
//
// Le funzione accettano come parametro la stringa stessa sulla quale scrivere il valore
// convertito, per non dover possedere la memoria utilizzata per l'output (char* puntatore).
// Per questo motivo ritorna invece la lunghezza.
//

extern "C"
const char* bsm_bool_to_str(bool value, size_t& len) 
{
    const char* str = value ? "true" : "false";
    len = value ? 4 : 5;
    return str;
}



#endif //_BSM_CONVERSIONS_