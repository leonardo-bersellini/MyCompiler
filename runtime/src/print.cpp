#ifndef _BSM_PRINT_
#define _BSM_PRINT_

#include <windows.h>
#include <stdint.h>

// FILE SORGENTE PER IL MODULO RUNTIME DI PRINT
//
// Questo modulo implementa la funzione di print tramite API Windows. (consoleApi).
// La funzione è dichiarata come extern C e non è dunque legata al linguaggio c++.
//

extern "C" 

// data: caratteri da stampare in stdout
// len: lunghezza del buffer
bool bsm_print(const char* data, size_t len)
{
    HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD written = 0;
    BOOL return_good = WriteConsoleA(Handle, data, len, &written, NULL);

    if(!return_good) {
        return false;
    }

    return true;
}


// BSM_PRINT_X
//
// Queste funzioni stampano valori che non sono stringhe, richiamando internamente
// funzioni runtime di conversione e lo stesso bsm_print.
//
// Contratto delle funzioni:
// Le seguenti sono richiamate dal modulo di codegen come funzioni linkate a runtime,
// devono quindi rispettare il contratto: <void> func_identifier(<T>) dove T è il tipo che va
// automaticamente convertito in stringa da stampare.

#include "conversions.cpp"

extern "C"
void bsm_print_char(char value)
{
    bsm_print(&value, 1);
}

extern "C"
void bsm_print_bool(bool value)
{
    size_t s;
    const char* str = bsm_bool_to_str(value, s);

    bsm_print(str, s);
}

#endif //_BSM_PRINT_