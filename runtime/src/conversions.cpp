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

// int32 -> char[]
size_t bsm_int32_to_str(int32_t value, char* out, size_t outCapacity)
{
    //caso negativo, conversione a -unsigned int 
    bool negative = value < 0;
    uint32_t uval = negative
        ? (static_cast<uint32_t>(-(value + 1)) + 1)  // gestione -INT_MIN
        : static_cast<uint32_t>(value);

    
    char tmp[10];   // un uint32_t ha al massimo 10 cifre decimali
    int digits = 0;

	// scrittura dei valori in ordine inverso
    do {
        tmp[digits++] = '0' + static_cast<char>(uval % 10);
        uval /= 10;
    } while (uval != 0);

    // scrittura nel buffer di output, i valori vanno riordinati
    size_t len = 0;

    if (negative && len < outCapacity)
        out[len++] = '-';

    while (digits > 0 && len < outCapacity)
        out[len++] = tmp[--digits];

    return len;
}

// double -> char[]
// precisione fissa decimale: 6 cifre (troncate)
size_t bsm_double_to_str(double value, char* out, size_t outCapacity)
{
    constexpr int PRECISION = 6;

    //gestione valore negativo
    bool negative = value < 0.0;
    double absValue = negative ? -value : value;

    //divisone di parte intera e decimale
    uint64_t intPart = static_cast<uint64_t>(absValue);
    double fracPart = absValue - static_cast<double>(intPart);

    //spostamento della prte decimale a sinistra della virgola
    uint64_t scale = 1;
    for (int i = 0; i < PRECISION; ++i) scale *= 10;   // scale = 10^PRECISION

    uint64_t fracDigits = static_cast<uint64_t>(fracPart * static_cast<double>(scale));

    //scrittura del numero nel buffer
    size_t len = 0;
    if (negative && len < outCapacity)
        out[len++] = '-';

    char tmp[20];
    int digits = 0;
    uint64_t v = intPart;
    do {
        tmp[digits++] = '0' + static_cast<char>(v % 10);
        v /= 10;
    } while (v != 0);

    while (digits > 0 && len < outCapacity)
        out[len++] = tmp[--digits];

    //inserimento del punto decimale
    if (len < outCapacity)
        out[len++] = '.';

    //buffer per cifre decimali, riempito con '0' per valori mancanti
    char fracTmp[PRECISION];
    for (int i = PRECISION - 1; i >= 0; --i) {
        fracTmp[i] = '0' + static_cast<char>(fracDigits % 10);
        fracDigits /= 10;
    }

    for (int i = 0; i < PRECISION && len < outCapacity; ++i)
        out[len++] = fracTmp[i];

    return len;
}

// bool -> char[]
extern "C"
const char* bsm_bool_to_str(bool value, size_t& len) 
{
    const char* str = value ? "true" : "false";
    len = value ? 4 : 5;
    return str;
}



#endif //_BSM_CONVERSIONS_