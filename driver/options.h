#ifndef OPTIONS_H
#define OPTIONS_H

#include <QString>

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
    QString inputFile;
    QString outputFile;
    OutputKind outkind = OutputKind::Executable;

    //flag opzionali aggiuntive
    bool emitIR = false;
    bool verbose = false;
};

#endif // OPTIONS_H
