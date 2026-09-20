#ifndef LINKER_H
#define LINKER_H

#include <string>

#include "utils/ansi/ansi.h"
namespace clr = ansi::color;

/*
 * Questa funzione si occupa di utilizzare il linker del progetto per costruire un eseguibile,
 * linkando i file oggetto indicati.
 * Si utilizza il linker lld-link.exe di msys64-ucrt64
 * il flag debug è impostato nella chiamata da parte di compilerdriver. (in base alle opzioni verbose).
 */
namespace linker 
{
    bool lld_link(const std::string &objFile, const std::string &outputExe, bool debug);
}

#endif //LINKER_H