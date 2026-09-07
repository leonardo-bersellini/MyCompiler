#ifndef SCOPE_STACK_H
#define SCOPE_STACK_H

#include "template_stack.h"

#include <optional>

template<typename A, typename B>

class scope_stack : public Stack<A, B>
{
public:

    /*
     * Permette di dichiarare un simbolo, inserendolo nello scope corrente, che
     * corrisponde all'ultimo della lista.
     */
    void declareSymbol(const A& first, const B& second) override {
        this->m_stack.back().insert({first, second});
    }

    /*
     * Cerca l'elemento specificato in tutto lo stack, per poi restituire
     * le informazioni di quel simbolo.
     * Ritorna un valore opzionale, nullopt se non trovato nello stack
     */
    virtual std::optional<B> lookupSymbol(const A& key) const override {
        for(int i = this->m_stack.size() - 1; i >= 0; i--) {
            if (this->m_stack[i].contains(key)) return this->m_stack[i].at(key);
        }
        return std::nullopt; // non trovato
    }

    /*
     * Aggiunge un elemento vuoto come nuovo livello all'interno dello stack
     */
    void push() override {
        this->m_stack.push_back(std::unordered_map<A, B>());
    }

    /*
     * Rimuove l'ultimo livello dello stack uscendo dallo scope attuale
     */
    void pop() override {
        this->m_stack.pop_back();
    }

    /*
    * Controlla l'esistenza di un simbolo all'interno di tutto lo stack degli scope
    * presenti.
    */
    bool symbolExistsAnywhere(const A& key) const {
        for(int i= this->m_stack.size() -1; i >= 0; i--) {
            if(this->m_stack[i].contains(key)) return true;
        }
        return false;
    }

    /*
     * Controlla l'esistenza di un simbolo solo nello scope corrente, che
     * corrisponde all'ultimo scope dello stack.
     */
    bool symbolExistsInCurrentScope(const A& key) const {
        if(this->m_stack.back().contains(key))
            return true;
        return false;
    }

    /*
     * Permette di controllare se lo scope corrente è quello globale, ovvero
     * se corrisponde al primo livello dello stack.
     */
    bool isGlobalScope() const
    {
        return this->m_stack.size() == 1;
    }

    /*
     * rimuove tutti gli elementi dello stack
     */
    void clear() {
        this->m_stack.clear();
    }


};

#endif //SCOPE_STACK_H