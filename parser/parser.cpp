#include "parser.h"

#include <memory>
#include "keywords.h"

Parser::Parser() {}

Token Parser::peek(int offset) const
{
    int position = currentPos + offset;
    if(isAtEnd(position)) return tokens.last();
    return tokens.at(position);
}

Token Parser::advance() {
    if(isAtEnd()) return tokens.last();
    Token current = tokens.at(currentPos);
    currentPos++;
    return current;
}

bool Parser::isAtEnd() const {
    if(currentPos >= tokens.size()) return true;
    return (tokens.at(currentPos).type == TokenType::EndOfFile);
}

bool Parser::isAtEnd(int pos) const {
    if(pos >= tokens.size()) return true;
    return (tokens.at(pos).type == TokenType::EndOfFile);
}

/*
 * Restituisce true se il tokentype assegnato ad essa corrisponde al token attuale della lista.
 */

bool Parser::check(TokenType type) const {
    return (tokens.at(currentPos).type == type);
}

/*
 * Restutuisce true se il tokentype assegnato ad essa corrisponde al token futuro nella lista.
 * In caso contrario, restituisce false ed aggiunge un log di errore.
 */

bool Parser::expect(TokenType type) {
    if(peek().type == type) {
        advance(); //consuma il token atteso
        return true;
    } else {
        Token t;
        t.type = type;
        errorLog->addError("Expected " + typeToString(t.type) + " before " + typeToString(peek().type),
                           tokens.at(currentPos).position);
        advance(); //error recovery, salta il token errato
    }
    return false;
}

/*
 * Punto di ingresso per il parsing del programma.
 * si occupa di elaborare e conservare gli statements corrispondenti ai token in un oggetto *<program>
 */

std::unique_ptr<Program> Parser::parseProgram(const QList<Token>& tokens, ErrorLog &errorLog)
{
    this->tokens = tokens;
    this->errorLog = &errorLog;
    currentPos = 0;

    std::unique_ptr<Program> program = std::make_unique<Program>(); //contenitore del programma parsato

    while(!isAtEnd())
    {
        program->statements.push_back(parseStatement());
    }

    return program;
}

/*
 * Funzione di parsing di ogni statement.
 * Rispecchiando la grammatica del linguaggio, interpreta le sequenze di token in modo da
 * formare degli statements, controllando la presenza di terminatori e caratteri di grammatica.
 * In sintesi si occupa di riconoscere quale tipo di stmt corrisponde ai token presenti, 
 * in modo da instradare la creazione del corretto tipo di oggetto_stmt.
 */

std::unique_ptr<Stmt> Parser::parseStatement()
{
    if(check(TokenType::Identifier) && peek(1).type == TokenType::Equal)
    {
        // Assegnazione

        QString name = advance().lexeme; //consuma l'identificatore
        advance();                       //consuma '='
        auto expr = parseExpression();   //rimangono i token dell'espressione
        expect(TokenType::Semicolon);    //expect ; after

        //ritorna uno stmt di assegnazione
        auto stmt = std::make_unique<AssignmentStmt>();
        stmt->name = name;
        stmt->value = std::move(expr);
        return stmt;
    }
    else if(check(TokenType::TypeKeyword))
    {
        if(peek(2).type == TokenType::Equal)
        {
            //dichiarazione con inizializzazione

            QString type = advance().lexeme;        // consuma TypeKeyword
            QString name = peek().lexeme;           // legge il nome presumendo che sia un identifier
            bool isValid = expect(TokenType::Identifier); // verifica identifier e lo consuma

            if(!isValid) {
                /* Ritorna un errorStmt se la variabile non corrisponde ad un identifier,
                 * in modo da evitare che sia costruita una variabile con un token invalido */
                return std::make_unique<ErrorStmt>();;
            }

            advance();      //consuma '=' se l'identifier è valido

            auto initExpr = parseExpression();
            expect(TokenType::Semicolon);

            auto d = std::make_unique<DeclarationStmt>();
            d->type = toValueType(type); //QString -> ValueType
            d->name = name;
            d->initializer = std::move(initExpr);
            return d;

        } else {
            //dichiarzione pura
            QString type = advance().lexeme;  //consuma TypeKeyword
            QString name = advance().lexeme;   // consuma Identifier
            expect(TokenType::Semicolon);

            auto d = std::make_unique<DeclarationStmt>();
            d->type = toValueType(type); //QString -> ValueType
            d->name = name;
            d->initializer = nullptr; //nessun initializer
            return d;
        }

    } else {
        // Espressione
        auto expr = parseExpression();   //risolve direttamente l'espressione
        expect(TokenType::Semicolon);

        //ritorna uno stmt di espressione
        auto stmt = std::make_unique<ExpressionStmt>();
        stmt->expr = std::move(expr);
        return stmt;
    }
}

/**
 * FUNZIONI DI PARSING
 * Questa funzioni sono chiamate in ordin di precedenza a seguito di parseStmt.
 * In base all'ordine di precedenza degli operatori, si controlla in ogni funzione se è presente
 * l'operatore a cui essa è dedicata. Le prime funzioni ad essere chiamate sono, per struttura, quelle
 * corrispondenti agli operatori con precedenza più bassa. La precedenza è indicata dall'ordine di
 * scrittura delle funzioni, le prime nel documento da questo puntoc corrispondono alla precedenza più
 * bassa, come già detto.
 * La precedenza è garantita dalla struttura delle espressioni che, prima di controllare se il 'loro'
 * operatore/i è presente, richiamano la funzione al livello sottostante. questo garantisce che le
 * funzioni più in basso nella catena siano eseguite per prime.
 * END
 *
 * nota: se si desidera modificare la funzione a top-level della catena (corrispondente alla più
 * bassa precedenza), allora sarà necessario sostituire tutte le chiamate a quella stessa funzione con
 * le chiamate alla nuova in parseStmt, che da il via alla catena.
 *
 * nota: tutte le funzioni ritornano un ptr ad un Expr, questo oggetto però indica una qualsiasi
 * espressione di codice, diversa dal concetto di espressione della funzione parseExpression, che indica
 * un'espressione matematica.
 **/

/*
 * Punto di ingresso delle funzioni di parsing per operatori logici.
 * Questa funzione si occupa di effettuare il parsing dell'operatore OR [!] (se presente),
 * in caso contrario, si scende ai prossimi livelli di controllo logico.
 */

std::unique_ptr<Expr> Parser::parseLogicalOr()
{
    auto left = parseLogicalAnd();
    
    while(check(TokenType::LogicalOr)) {
        TokenType op = advance().type;
        auto right = parseLogicalAnd();

        auto binExpr = std::make_unique<BinaryExpr>();
        binExpr->op = op;
        binExpr->left = std::move(left);
        binExpr->right = std::move(right);
        left = std::move(binExpr);
    }

    return left;
}

/*
 * Questa funzione si occupa di svolgere il parsing dell'operatore AND logico.
 * operatore [&&]
 */

std::unique_ptr<Expr> Parser::parseLogicalAnd()
{
    auto left = parseComparison();

    while(check(TokenType::LogicalAnd)) {
        TokenType op = advance().type;
        auto right = parseComparison();

        auto binExpr = std::make_unique<BinaryExpr>();
        binExpr->op = op;
        binExpr->left = std::move(left);
        binExpr->right = std::move(right);
        left = std::move(binExpr);
    }

    return left;
}

/*
 * Questa funzione si occupa di svolgere il parsing delle espressioni
 * contenenti operatori di confronto [ >, <, >=, <=, ==, != ].
 */

std::unique_ptr<Expr> Parser::parseComparison()
{
    auto left = parseExpression();

    while (check(TokenType::EqualEqual) || check(TokenType::Less) || check(TokenType::Greater)
           || check(TokenType::LessEqual) || check(TokenType::GreaterEqual) || check(TokenType::NotEqual)) {
        TokenType op = advance().type;
        auto right = parseExpression();

        auto binExpr = std::make_unique<BinaryExpr>();
        binExpr->op = op;
        binExpr->left = std::move(left);
        binExpr->right = std::move(right);
        left = std::move(binExpr);
    }

    return left;
}

/*
 * Funzione di parsing delle espressioni matematiche.
 * Questa funzione si occupa di avviare il parsing delle espressioni, richiamando anche sottofunzioni
 * con scopo ricorsivo (per rispettare precedenze di operatori matematici).
 * Questo blocco corrisponde quindi al livello più alto di lettura di un'espressione matematica e,
 *  di conseguenza, al livello più basso di precedenza matematica (poichè è il primo ad essere letto).
 */

std::unique_ptr<Expr> Parser::parseExpression()
{
    auto left = parseTerm();
    
    // Livello più basso di precedenza matematica [+ , -]
    while(check(TokenType::Plus) || check(TokenType::Minus)) {
        TokenType op = advance().type; //consuma l'operatore
        auto right = parseTerm();
        
        // operazione binaria che unisce il calcolo di un operando destro e sinistro
        auto binaryExpr = std::make_unique<BinaryExpr>();
        binaryExpr->op = op;
        binaryExpr->left = std::move(left);
        binaryExpr->right = std::move(right);
        
        left = std::move(binaryExpr); //la variabile viene utilizzata come ritorno.        
    }
    
    return left;
}

/*
 * Funzione di parsing dei termini di un'espressione.
 * Questa funzione si occupa di eseguire il parsing dei termini di un'espressione, quindi 
 * delle componenti di un'espressione con un livello di annidamento.
 * Per termine si intende una coppia di fattori uniti da un'operazione, questa funzione risolve
 * quell'operazione, che per corrisponde ad un livello di precedenza matematica superiore al precedente.
 */

std::unique_ptr<Expr> Parser::parseTerm()
{
    auto left = parseFactor();
    
    // Livello superiore di precedenza matematica [* , /]
    while(check(TokenType::Star) || check(TokenType::Slash)) {
        TokenType op = advance().type; //consuma l'operatore
        auto right = parseFactor();
        
        //costruzione di un'altra espressione binaria annidata
        auto binaryExpr = std::make_unique<BinaryExpr>();
        binaryExpr->op = op;
        binaryExpr->left = std::move(left);
        binaryExpr->right = std::move(right);
        
        left = std::move(binaryExpr);
    }
    
    return left;
}

/*
 * Funzione di parsing dei fattori di un termine.
 * Questa funzione si occupa di eseguire il parsing dei singoli fattori di un'espressione, che sono le
 * componenti più annidate di un'espressione, corrispondenti al livello più alto di precedenza matematica.
 * Questa funzione genera la ricorsione nel processo di parsing delle espressioni, poichè richiama funzioni
 * a livelli più esterni in caso di operatori come parentesi.
 * In questo blocco viene eseguita anche la gestione degli errori di sintassi.
 */

std::unique_ptr<Expr> Parser::parseFactor()
{
    //controllo in base al tipo di token che si deve gestire

    // Numeri letterali
    if(check(TokenType::IntegerLiteral) || check(TokenType::DoubleLiteral))
    {
        Token t = advance();

        auto numberExpr = std::make_unique<NumberExpr>();
        numberExpr->value = t.numericValue;
        numberExpr->isInteger = (t.type == TokenType::IntegerLiteral);
        return numberExpr;
    }

    // Stringhe letterali
    else if(check(TokenType::StringLiteral))
    {
        QString lexeme = advance().lexeme;

        auto strExpr = std::make_unique<StringExpr>();
        strExpr->value = lexeme;
        return strExpr;
    }

    // Char letterali
    else if(check(TokenType::CharLiteral))
    {
        QString ch_str = advance().lexeme; //stringa con un carattere

        auto charExpr = std::make_unique<CharExpr>();
        charExpr->value = ch_str.at(0); //estrazione char
        return charExpr;
    }

    // Booleano Letterale
    else if(check(TokenType::BoolLiteral))
    {
        QString lexeme = advance().lexeme; //consuma il token valore
        bool val;

        if(keywords.contains(lexeme)) {
            //valore assegnato come "true" o "false"
            val = lexeme == "true" ? true : false;
        } else {
            //valore assegnato come 0 o 1
            val = lexeme == "1" ? true : false;
        }

        auto boolExpr = std::make_unique<BooleanExpr>();
        boolExpr->value = val;
        return boolExpr;
    }

    // Varibile
    else if(check(TokenType::Identifier))
    {
        auto name = advance().lexeme;

        auto variableExpr = std::make_unique<VariableExpr>();
        variableExpr->name = name;
        return variableExpr;
    }

    // Parentesi
    else if(check(TokenType::LParen))
    {
        advance(); //consuma il token '('

        auto expr = parseExpression(); //call ricorsiva
        expect(TokenType::RParen);
        return expr;
    }

    // Operatori unari - il + è considerato solo per non generare errore
    else if(check(TokenType::Minus) || check(TokenType::Plus))
    {
        TokenType op = advance().type; //consuma il token '-' o '+'

        auto unaryExpr = std::make_unique<UnaryExpr>();
        unaryExpr->op = op;
        unaryExpr->operand = parseFactor(); //call ricorsiva
        return unaryExpr;
    }

    // Errori di sintassi
    else {
        errorLog->addError("Parse error. Expected a factor, received a different token", peek().position);
        advance(); //error recovery, salta il token errato
        auto error = std::make_unique<ErrorExpr>();
        return error;
    }

    return nullptr;
}