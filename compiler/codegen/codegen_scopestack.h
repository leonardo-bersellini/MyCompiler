#ifndef CODEGEN_SCOPE_STACK_H
#define CODEGEN_SCOPE_STACK_H

#include <string>
#include <llvm/IR/Instructions.h>

#include "utils/stack/scope_stack.h"

/*
 * Questo header contiene una classe che funge da implementazione diretta della base
 * scope_stack. Questo è necessario per implementare un metodo che differisce dalla classe base.
 * Non sarebbe quindi possibile riutilizzare lo scope_stack direttamente nel codegen (creando un membro
 * utilizzando i typename dinamici), ma bisogna creare una classe intermedia che, per semplicità e 
 * correttezza, implementa direttamente i typename necessari per il codegen.
 */

class AllocaScopeStack : public scope_stack<std::string, llvm::AllocaInst*>
{
public:

    /*
    * Cerca un simbolo per nome, scorrendo lo stack in ordine inverso: dallo scope più
    * interno (corrente) verso quello più esterno, per rispettare lo shadowing.
    * Se non trovato in nessun livello, il programma lancia un eccezione. Si tratta di un
    * bug interno del compiler, l'analisi semantica avrebbe dovuto bloccarlo.
    */
    std::optional<llvm::AllocaInst*> lookupSymbol(const std::string& name) const override 
    {
        for (auto it = this->m_stack.rbegin(); it != this->m_stack.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }

        throw std::runtime_error("codegen internal error: undeclared symbol '" + name + "'");
        return std::nullopt;
    }

};

#endif //CODEGEN_SCOPE_STACK_H