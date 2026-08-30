#include "recoveryhandler.h"

/*
 *  Error Recovery Handler
 *   
 *  Questa classe si occupa di gestire diversi metodi di recovery degli errori, necessari
 *  per poter proseguire l'esecuzione oltre al primo errore.
 *  Le pratiche di recovery sono richiamate dall'esecuzione del parser.
 *  L'handler si occupa di eseguire la recovery sui tokens (lista condivisa con il 
 *  parser per riferimento), in base alle direttive del parser.
 */

RecoveryHandler::RecoveryHandler(ErrorLog*& log, std::vector<Token>& tokens, int& currentPos)
    : errorLog(log), m_tokens(tokens), m_index(currentPos)
{}

/*
 *  ErrorRecovery: Panic-Mode
 *  La recovery per sincronizzazione è detta panic mode. Si applica quando si perde
 *  il contesto dell'errore attuale, allora si deve proseguire l'esecuzione dal primo
 *  token "sicuro" che si trova (es: ';' '{' '}'), detto token di sync.
 */

void RecoveryHandler::synchronize(const std::vector<TokenType>& syncPoints)
{
    while(m_tokens.at(m_index).type != TokenType::EndOfFile)
    {
        bool isSyncPoint = false;

        for(const auto& sync : syncPoints) 
        {
            auto current = m_tokens.at(m_index);
            if(current.type == sync) {
                isSyncPoint = true;
                break;
            }
        }

        if(isSyncPoint) break; //stop del while

        m_index++; //consuma i token fino al sync
    }

    if(m_tokens.at(m_index).type == TokenType::EndOfFile) {
        errorLog->addWarning("error recovery failed: next errors may be unreliable");
    }

    m_index++;

    return;
}

/*
 *  ErrorRecovery: skip
 *  Questa recovery salta il token corrente. Utile per errori di scrittura o typo minimali.
 *  Permette di saltare velocemente errori piccoli.
 */

void RecoveryHandler::skipToken() 
{
    if(m_tokens.at(m_index + 1).type == TokenType::EndOfFile) {
        errorLog->addWarning("error recovery failed: next errors may be unreliable");
    }

    m_index++;

    return;
}

/*
 *  ErrorRecovery: ghost
 *  La logica di base di questa recovery è la stessa di quella di skip.
 *  Cambia però il metodo di esecuzione, per evitare errori piccoli si finge di aver trovato
 *  il token necessario all'esecuzione. Utile soprattutto quando i token possibili in quel 
 *  contesto sono molto limitati.
 */

void RecoveryHandler::ghostToken(TokenType type)
{
    Token t;
    t.type = type;
    t.lexeme = "<ghost>";
    t.position = m_tokens.at(m_index).position;

    m_tokens.insert(m_tokens.begin() + m_index, t);

    m_index++; //consuma tkn ghost
}