#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>

/**
 * OPZIONI DI COMPILAZIONE
 * Questo header racchiude tutte le strutture necessarie per rappresentare le opzioni
 * con cui è stato invocato il comando da terminale.
 */

enum class OutputKind
{
    Executable,  //default
    ObjectFile,
};

// opzioni con cui il compiler è stato invocato da cli

struct CompilerOptions
{
    std::string inputFile;
    std::string outputFile;
    OutputKind outkind = OutputKind::Executable;

    //flag opzionali aggiuntive
    bool emitIR = false;
    bool verbose = false;
    bool noOutputFile = false;
    bool hideWarnings = false;
};

#endif // OPTIONS_H
