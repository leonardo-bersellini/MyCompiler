#ifndef _BSM_PRINT_
#define _BSM_PRINT_

#include <windows.h>

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



#endif //_BSM_PRINT_