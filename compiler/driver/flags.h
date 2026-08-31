#ifndef FLAGS_H
#define FLAGS_H

#include <string>
#include <vector>

#include "options.h"

/**
 * FLAG DI TIPO 1: PIPELINE FLAG
 * Sintassi: -<flagname>
 * Queste flag indicano al compiler DOVE fermare il processo di esecuzione oppure
 * quale metodo di compilazione usare. (per esempio per fermare la compilazione ad uno
 * step intermedio).
 * Per stuttura queste flag possono portarsi un valore che modifichi le opzioni di compilazione.
 *
 * Nota: i nomi di questi flag devono essere di lunghezza pari a 1
 */

struct PipelineFlag {
    std::vector<std::string> names;
    bool requiresValue;
    std::string description;
    OutputKind resultingKind;
    std::string CompilerOptions::* valueTarget; //nullptr se non serve un valore
};

inline const std::vector<PipelineFlag> pipelineFlags = {
    { {"o"}, true, "Compile and assemble, but do not link (produces .obj)", OutputKind::ObjectFile, &CompilerOptions::outputFile},
    { {"e"}, true, "Generate an executable from source (produces .exe)", OutputKind::Executable, &CompilerOptions::outputFile},
};

/**
 * FLAG DI TIPO 2: UTILITY FLAG
 * Sintassi: --<flagname>
 * Questo gruppo di flag, come detto dal nome, servono come utility per l'utente da cli.
 * Il loro scopo è indicare al compiler COME arrivare alla destinazione selezionata, ad esempio
 * specificando che tipo di output mostrare a terminale o se indicare tutti i passaggi in ouput.
 *
 * Nota: i nomi di questi flag devono essere di lunghezza superiore a 1 (meglio se superiore a 3-4).
 */

struct UtilityFlag {
    std::vector<std::string> names;
    std::string description;
    bool CompilerOptions::* target;
};

inline const std::vector<UtilityFlag> utilityFlags = {
    { {"IR", "llvm-ir"}, "Print generated LLVM IR to stdout", &CompilerOptions::emitIR },
    { {"Vb", "verbose"}, "Show detailed information during the execution.", &CompilerOptions::verbose},
    { {"no-generation"}, "Execute the compiler process without generating any file as output", &CompilerOptions::noOutputFile},
    { {"Wh", "hide-warnings"}, "Hide all the warnings collected during execution", &CompilerOptions::hideWarnings},
};


#endif // FLAGS_H
