#ifndef NAMESPACE_TABLE_H
#define NAMESPACE_TABLE_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <optional>
#include <iostream>

/*
 *  Namespaces Table
 * Questa tabella è utilizzata per mantenere le informazioni sui namespace analizzati,
 * per eseguire lookup o controlli sentici.
 * A differenza degli stack o di altre tabelle, il pop non comporta una perdita di dati,
 * ma solo il cambiamento del namespace corrente dove dichiarare simboli. Questo in modo
 * da non perdere dati che andranno riutilizzati anche dopo la definizione del namespace.
 * 
 * La struttura della table in questo caso è un albero (ogni namespcae conosce figlio e genitore).
 * Si definisce table perchè chi utilicca questo oggetto ignora la struttra ad albero e la utilizza
 * indipendentemente come una table di dati.
 */

/*
 *  Struttura rispetto al compiler
 * Dal punto di vista architetturale, la tabella dei namespace è condivisa tra analisi semantica
 * e codegen. L'analisi semantica compila la tabella, mentre il codegen la legge e la modifica 
 * parzialmente per mangling dei nomi.
 * A questo scopo, i file che faranno uso di questa tabella dovranno includere l'header apposito 
 * in questa cartella di utility che instanzierà un typename (corrispondente alla tabella con il
 * template corretto risolto).
 */

/*
 *  Mangled names
 * Regole di creazione di un nome mangled (nome univoco che rappresenta un namespace 
 * specificando a quali parents appartiene).
 * il nome inizia per NS_ (namespace), ed ogni elemento è diviso da un carattere $.
 * es: a::b::f() => NS_a$b$f;
 */

template<typename A, typename B>
struct NamespaceNode //singolo namespace
{
    NamespaceNode(const std::string& n) : name(n) {} 

    std::string name;
    NamespaceNode* parent = nullptr;
    std::unordered_map<std::string, std::unique_ptr<NamespaceNode>> childs;
    std::unordered_map<A, B> symbolTable;
};


//i due tipi del template devono essere i tipi del pair considerato come symbol, 
//poichè descrivono il tipo di un simbol salvato nella symboltable di ogni namespace.
template<typename A, typename B>

class namespace_table 
{
public:

    /*
     * Dichiara un symbol dentro al namespace corrente.
     */
    void declareSymbol(const A &first, const B &second) {
        if(!currentNamespace) throw std::runtime_error("invalid use of namespace table. currentNamespace => nulltpr");

        currentNamespace->symbolTable.insert({first, second});
    }

    /*
     * Cerca un simblo nel namespace corrente, ritornando un valore opzionale.
     */
    std::optional<B> lookupSymbol(const A &key) const {
        if(!currentNamespace) return std::nullopt;

        if(currentNamespace->symbolTable.contains(key)) {
            return currentNamespace->symbolTable.at(key);
        }

        return std::nullopt;
    }

    /*
     * Ritorna true se il simbolo corrispondente alla chiave indicata esiste nel namespace corrente.
     */
    bool symbolExistInCurrentNamespace(const A &key) const {
        if(!currentNamespace) return false;

        if(currentNamespace->childs.contains(key)) return true;
        if(currentNamespace->symbolTable.contains(key)) return true;

        return false;
    }

    /*
     * Risolve un controllo richiesto su nomi qualified (es. a::b dentro il namespace corrente).
     * Ritorna un true se il valore richiesto è valido come namespace, partendo da un livello
     * globale. Se il namespace non viene trovato, allora il qualified name corrisponde ad una
     * richiesta semanticamente incorretta.
     */
    bool searchQualifiedName(const std::vector<std::string>& names) const
    {
        std::size_t i = 0;

        if(!m_namespaces.contains(names.at(i))) return false;

        const Namespace* n = m_namespaces[names[i]].get();
        ++i;

        for(std::size_t j = i; j < names.size(); ++j) 
        { 
            if(!n->childs.contains(names.at(j))) return false;

            n = n->childs[names[j]].get();
        }

        return true;
    }

    /*
     * Aggiunge un nuovo namespace alla tabella. Il nuovo namespace sarà a livello globale
     * se non ci si trova dentro un altro namespace, o nested dentro quest'ultimo altrimenti.
     * Inoltre il namespace corrente sarà il nuovo namespace, spostando l'esecuzione ed i controlli
     * sul nuovo "livello" aggiunto. 
     */
    void push(const std::string& name) 
    {
        if(!currentNamespace) {
            auto n = std::make_unique<Namespace>(name);
            currentNamespace = n.get();
            m_namespaces.insert({n->name, std::move(n)});
            return;
        }

        const auto got = getNamespace(name);

        if(got) {
            currentNamespace = got;
        } else {
            auto n = std::make_unique<Namespace>(name);
            n->parent = currentNamespace;

            Namespace* newNamespace = n.get();

            currentNamespace->childs.insert({n->name, std::move(n)});

            currentNamespace = newNamespace;
        }

        return;
    }

    /*
     * Esegue il pop del livello corrente della table, spostando il namespace corrente
     * al suo nodo genitore, se esiste.
     */
    void pop() {
        if(!currentNamespace) return;

        if(!currentNamespace->parent) {
            currentNamespace = nullptr;
            return;
        }

        currentNamespace = currentNamespace->parent;
    }

    /*
     * Restituisce un nome mangled a partire da un nodo e dal suo nome puro.
     * Permette di ottenere un nome da utilizzare nella generazione del codice (salvataggio di 
     * nomi mangled per namespace) in automatico confrontando i dati dei nodi.
     */
    std::string mangleName(const std::string &name) const 
    {
        if(!currentNamespace) return name;

        //accumula gli identificatori dal nodo selezionato fino al parent più esterno
        std::vector<std::string> segments;
        for(Namespace* n = currentNamespace; n != nullptr; n = n->parent) {
            segments.push_back(n->name); 
        }

        //ricostruisce il nome mangled in ordine corretto
        std::string result = "NS_";

        for(auto s = segments.rbegin(); s != segments.rend(); s++) {
            result += *s + "$";
        }
        result += name;

        return result;
    }
    
private:
    using Namespace = NamespaceNode<A, B>;

    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_namespaces;
    Namespace* currentNamespace = nullptr;

    // ritorna un pointer al namespace richiesto se esiste 
    Namespace* getNamespace(const std::string& name) 
    {
        if(!currentNamespace) {
            if(m_namespaces.contains(name)) {
                return m_namespaces.at(name).get();
            }
            return nullptr;
        }

        //cerca solo tra i figli diretti (sintassi namespace)
        if(currentNamespace->childs.contains(name)) {
            return currentNamespace->childs[name].get();
        }

        return nullptr;
    }
    
};



#endif //NAMESPACE_TABLE_H
