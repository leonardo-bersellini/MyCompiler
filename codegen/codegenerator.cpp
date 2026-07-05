#include "codegenerator.h"

CodeGenerator::CodeGenerator()
{}

/**
 * CODE GENERATION
 * Il codice viene generato utilizzando il framework llvm.
 * La classe CodeGenerator (this) funge da visitor dell'ast prodotto, che viene convertito
 * in codice llvm ir e llvm.
 *
 * Il codice passato al generatore non deve contenere error stmt, per una questione pratica e logica.
 * Per struttura stessa della codegen, infatti, il codice non può essere creato con errori; da questo
 * deriva il fatto che il generatore non prevede azioni per errorStmt o simili, eccetto controlli
 * di sicurezza.
 */


