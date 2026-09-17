#ifndef CODEGEN_SCOPE_STACK_H
#define CODEGEN_SCOPE_STACK_H

#include <string>
#include <unordered_map>

#include <llvm/IR/Value.h>

#include "utils/stack/scope_stack.h"

/*
 * Questo header contiene una classe che funge da implementazione diretta della base
 * scope_stack. Questo è necessario per implementare un metodo che differisce dalla classe base.
 * Non sarebbe quindi possibile riutilizzare lo scope_stack direttamente nel codegen (creando un membro
 * utilizzando i typename dinamici), ma bisogna creare una classe intermedia che, per semplicità e 
 * correttezza, implementa direttamente i typename necessari per il codegen.
 */

class AllocaScopeStack : public scope_stack<std::string, llvm::Value*>
{
public:

    /*
    * Cerca un simbolo per nome, scorrendo lo stack in ordine inverso: dallo scope più
    * interno (corrente) verso quello più esterno, per rispettare lo shadowing.
    * Se non trovato in nessun livello, il programma lancia un eccezione. Si tratta di un
    * bug interno del compiler, l'analisi semantica avrebbe dovuto bloccarlo.
    */
    std::optional<llvm::Value*> lookupSymbol(const std::string& name) const override 
    {
        for (auto it = this->m_stack.rbegin(); it != this->m_stack.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }

        auto g = lookupGlobal(name);
        if(g) return g;

        throw std::runtime_error("codegen internal error: undeclared symbol '" + name + "'");
        return std::nullopt;
    }

    /*
     * Metodo che permette la dichiarazione di una variabile globale llvm, salvata
     * nella mappa interna dedicata alle globals
     */
    void declareGlobal(const std::string& name, llvm::GlobalVariable* g) {
        globals.insert({name, g});
    }

    /*
     * Lookup specifico per la mappa delle variabili globali.
     */
    llvm::GlobalVariable* lookupGlobal(const std::string& name) const {
        auto it = globals.find(name);
        return it != globals.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<std::string, llvm::GlobalVariable*> globals;

};

#endif //CODEGEN_SCOPE_STACK_H