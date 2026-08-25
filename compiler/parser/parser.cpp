#include "parser.h"

#include <memory>
#include <iostream>
#include "keywords.h"

Parser::Parser() {}

Token Parser::peek(int offset) const
{
    int position = currentPos + offset;
    if(isAtEnd(position)) return tokens.back();
    return tokens.at(position);
}

Token Parser::advance() {
    if(isAtEnd()) return tokens.back();
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

std::unique_ptr<Program> Parser::parseProgram(const std::vector<Token>& tokens, ErrorLog &errorLog)
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
 * Funzione di parsing di ogni statement. [entry point del parsing stmt]
 * Rispecchiando la grammatica del linguaggio, interpreta le sequenze di token in modo da
 * formare degli statements, controllando la presenza di terminatori e caratteri di grammatica.
 * In sintesi si occupa di riconoscere quale tipo di stmt corrisponde ai token presenti, 
 * in modo da instradare la creazione del corretto tipo di oggetto_stmt.
 */

std::unique_ptr<Stmt> Parser::parseStatement()
{
    if(check(TokenType::LBrace))
    {
        // Nuovo Scope
        return parseScopeStmt();
    }
    else if(check(TokenType::Identifier) && peek(1).type == TokenType::Equal)
    {
        // Assegnazione

        return parseAssignStmt();
    }
    else if(check(TokenType::TypeKeyword))
    {
        //Dichiarazione o Funzione

        if(peek(2).type == TokenType::LParen)
            return parseFunctionStmt();
        else
            return parseDeclarationStmt();
    }
    else if(check(TokenType::ReturnKeyword))
    {
        // Return stmt

        return parseReturnStmt();
    }
    else if(check(TokenType::IfKeyword))
    {
        // If Statement

        return parseIfStmt();
    }
    else if(check(TokenType::WhileKeyword))
    {
        // While Statement

        return parseWhileStmt();
    }
    else if(check(TokenType::ForKeyword))
    {
        // For Statement

        return parseForStmt();

    }
    else if(check(TokenType::BreakKeyword))
    {
        // Break Instruction

        advance();                       // consuma 'break'
        expect(TokenType::Semicolon);    // consuma ';'
        return std::make_unique<BreakStmt>();

    }
    else if(check(TokenType::ContinueKeyword))
    {
        // Continue Instruction

        advance();                       // consuma 'continue'
        expect(TokenType::Semicolon);    // consuma ';'
        return std::make_unique<ContinueStmt>();

    } else {
        // Espressione

        auto expr = parseExpr();   //risolve direttamente l'espressione
        expect(TokenType::Semicolon);

        //ritorna uno stmt di espressione
        auto stmt = std::make_unique<ExpressionStmt>();
        stmt->expr = std::move(expr);
        return stmt;
    }

    return std::make_unique<ErrorStmt>();
}

/**
 * FUNZIONI DI PARSING <STMT>
 * Queste funzioni sottostanti hanno il compito di racchiudere la logica di gestione e creazione
 * di oggetti stmt, in modo da mantenere la struttura di dispatch generale (parseStmt) più pulita,
 * ed anche in modo da rendere ogni logica di parsing richiamabile da più punti.
 * END
 *
 * Nota: lo scopo pricipale è la pulizia del codice.
 */

/*
 * Funzione di parsing dei blocchi scope {...}.
 * Questa funzione si occupa del parsing di un nuovo scope, che ritorna uno stmt di tipo Block,
 * contenente una serie di altri stmt, ovvero le istruzioni da eseguire in quello scope.
 * In caso di errore, ritorna un errorStmt.
 */

std::unique_ptr<Stmt> Parser::parseScopeStmt()
{
    expect(TokenType::LBrace); // verifica e consuma '{'

    auto scope = std::make_unique<BlockStmt>();

    while(!isAtEnd() && peek().type != TokenType::RBrace)
    {
        scope->statements.push_back(parseStatement()); //ogni scope contiene un elenco di stmt
    }

    if(isAtEnd()) {
        errorLog->addError("Expected '}' before end of file", tokens.at(currentPos).position);
    } else {
        bool closed = expect(TokenType::RBrace);

        if(!closed) {
            auto s = std::make_unique<ErrorStmt>();
            return s;
        }
    }

    return scope;
}

/*
 *Questa funzione ritorna il parsing di uno scope, permettendo però come opzione valida
 * anche una sola istruzione senza {} branches per delimitare lo scope.
 * Questa struttura permette di scrivere singoli stmt al posto di uno scope
 * come istruzioni per delle keyword (if, for...).
 */

std::unique_ptr<Stmt> Parser::parseBranchBody()
{
    std::unique_ptr<Stmt> body;

    if(check(TokenType::LBrace))
        body = parseScopeStmt();
    else
        body = parseStatement();

    return body;
}

/*
 * Funzione di parsing degli stmt di assegnazione.
 * Caso diverso dall'assegnazione gestita in inizializzazione.
 */

std::unique_ptr<Stmt> Parser::parseAssignStmt()
{
    std::string name = advance().lexeme; //consuma l'identificatore
    advance();                       //consuma '='
    auto expr = parseExpr();   //rimangono i token dell'espressione
    expect(TokenType::Semicolon);    //expect ; after

    //ritorna uno stmt di assegnazione
    auto stmt = std::make_unique<AssignmentStmt>();
    stmt->name = name;
    stmt->value = std::move(expr);
    return stmt;
}

/*
 * Funzione di parsing degli stmt di dichiarazione.
 * Le dichiarazioni si dividono in "pure" e "con inizializzazione".
 * ovvero int x; e int x = 5;
 */

std::unique_ptr<Stmt> Parser::parseDeclarationStmt()
{
    if(peek(2).type == TokenType::Equal)
    {
        // --- Dichiarazione con inizializzazione --- //

        std::string type = advance().lexeme;        // consuma TypeKeyword
        std::string name = peek().lexeme;           // legge il nome presumendo che sia un identifier
        bool isValid = expect(TokenType::Identifier); // verifica identifier e lo consuma

        if(!isValid)
        {
            return std::make_unique<ErrorStmt>();;
        }

        advance();      //consuma '=' se l'identifier è valido

        auto initExpr = parseExpr();
        expect(TokenType::Semicolon);

        auto d = std::make_unique<DeclarationStmt>();
        d->type = Type::toValueType(type); //std::string -> ValueType
        d->name = name;
        d->initializer = std::move(initExpr);
        return d;
    }
    else
    {
        // --- Dichiarazione pura --- //
        std::string type = advance().lexeme;  //consuma TypeKeyword

        std::string name = peek().lexeme;     // legge Identifier
        bool isValid = expect(TokenType::Identifier);
        if(!isValid) return std::make_unique<ErrorStmt>();

        expect(TokenType::Semicolon);

        auto d = std::make_unique<DeclarationStmt>();
        d->type = Type::toValueType(type); //std::string -> ValueType
        d->name = name;
        d->initializer = nullptr; //nessun initializer
        return d;
    }
}

/*
 * Funzione di parsing degli stmt di tipo function.
 * Questi stmt corrispondono alle dichiarazioni di una funzione,
 * che accetta returntype, arguments e uno scope come body.
 */

std::unique_ptr<Stmt> Parser::parseFunctionStmt()
{
    ValueType returnType = Type::toValueType(advance().lexeme); // consuma tipo di ritorno
    std::string identifier = peek().lexeme; // consuma nome funzione
    bool valid = expect(TokenType::Identifier);

    if(returnType == ValueType::Error) errorLog->addError("error: returning <errortype> in function: " + identifier);
    if(!valid) return std::make_unique<ErrorStmt>();

    expect(TokenType::LParen);

    // --- parametri --- //
    std::vector<FunctionParam> params;

    while(!check(TokenType::RParen) && !isAtEnd())
    {
        ValueType paramType = Type::toValueType(advance().lexeme); // consuma tipo parametro
        std::string paramName = advance().lexeme;                 // consuma nome parametro

        params.push_back(FunctionParam{paramType, paramName});

        if(check(TokenType::Comma))
            advance(); // consuma ',' se c'è un altro parametro
    }

    expect(TokenType::RParen);

    auto body = parseScopeStmt();

    auto function = std::make_unique<FunctionStmt>();
    function->name = identifier;
    function->returnType = returnType;
    function->params = std::move(params);
    function->body = std::move(body);
    return function;

}

/*
 * Funzione di parsing degli stmt di return
 */

std::unique_ptr<Stmt> Parser::parseReturnStmt()
{
    advance(); //consuma 'return'

    auto r = std::make_unique<ReturnStmt>();

    if(check(TokenType::Semicolon)) {
        //return senza valore
        r->value = nullptr;
        advance();
    } else {
        r->value = parseExpr();
        expect(TokenType::Semicolon); //consuma ;
    }

    return r;
}

/*
 * Funzione di parsing degli stmt di tipo if.
 * Controlla anche la struttura elif/else, che annida un if dentro l'altro.
 */

std::unique_ptr<Stmt> Parser::parseIfStmt()
{
    advance();                      //consuma 'if' oppure 'elif'
    expect(TokenType::LParen);
    auto condition = parseExpr();   // Lettura della condizione tra parentesi
    expect(TokenType::RParen);

    // if body
    std::unique_ptr<Stmt> body;
    body = parseBranchBody();

    // --- elif --- //
    std::unique_ptr<Stmt> elseBranch = nullptr;

    if(check(TokenType::ElifKeyword)) {
        elseBranch = parseIfStmt(); //ricorsione e annidamento nuovo if
    }

    // --- else --- //
    else if(check(TokenType::ElseKeyword)) {
        advance(); //consuma 'else'

        elseBranch = parseBranchBody();
    }

    // --- nodo if --- //
    auto ifStmt = std::make_unique<IfStmt>();
    ifStmt->condition = std::move(condition);
    ifStmt->thenBranch = std::move(body);
    ifStmt->elseBranch = std::move(elseBranch);
    return ifStmt;
}

/*
 * Funzione di parsing degli stmt di tipo for.
 * esegue il controllo delle condizioni (init, cond, update) facoltative e crea un forstmt.
 *
 * Schema concettuale:
 * 'for' '(' init? ';' condition? ';' update? ')' body
 *
 * ognuno di questi elementi può essere assente (membri di forStmt nullptr).
 */

std::unique_ptr<Stmt> Parser::parseForStmt()
{
    advance(); //consuma 'for'

    auto forStmt = std::make_unique<ForStmt>();

    expect(TokenType::LParen);

    // --- controllo init --- //

    if(check(TokenType::Semicolon)) {
        advance(); //consuma ';'
    } else {
        auto init = parseStatement();
        forStmt->init = std::move(init);
    }

    // --- controllo condition --- //

    if(check(TokenType::Semicolon)) {
        advance();
    } else {
        auto cond = parseExpr();
        forStmt->condition = std::move(cond);

        expect(TokenType::Semicolon);
    }

    // --- controllo update --- //
    if(check(TokenType::RParen)) {
        advance();
    } else {
        auto update = parseExpr();
        forStmt->update = std::move(update);

        expect(TokenType::RParen);
    }

    auto body = parseBranchBody();
    forStmt->body = std::move(body);

    return forStmt;
}

/*
 * Questa funzione si occupa del parsing degli stmt dei cicli while.
 */

std::unique_ptr<Stmt> Parser::parseWhileStmt()
{
    advance(); //consuma 'while'

    expect(TokenType::LParen);
    auto cond = parseExpr();
    expect(TokenType::RParen);

    auto body = parseBranchBody();

    auto wStmt = std::make_unique<WhileStmt>();
    wStmt->condition = std::move(cond);
    wStmt->body = std::move(body);

    return wStmt;
}

/**
 * FUNZIONI DI PARSING <EXPR>
 * Questa funzioni sono chiamate in ordine di precedenza a seguito di parseStmt, tramite il
 * punto di ingresso parseExpr(), che ha il compito di instradare correttamente il parsing partendo
 * dal livello più esterno della catena.
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
 * bassa precedenza), allora sarà necessario sostituire la chiamate nell'entry-point parseExpr() con
 * la chiamata alla nuova funzione che si desidera aggiungere in cima alla catena.
 **/

/*
 * Punto di ingresso della catena di parsing degli oggetti <Expr>.
 * Ha il compito di richiamare la prima funzione della catena, fungendo da
 * entry point modificabile.
 * Richiama la prima funzione della catena di parsing<Expr>.
 */

std::unique_ptr<Expr> Parser::parseExpr()
{
    return parseLogicalOr();
}


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

    while(check(TokenType::LogicalAnd))
    {
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
    auto left = parseMathExpression();

    while (check(TokenType::EqualEqual) || check(TokenType::Less) || check(TokenType::Greater)
           || check(TokenType::LessEqual) || check(TokenType::GreaterEqual) || check(TokenType::NotEqual))
    {
        TokenType op = advance().type;
        auto right = parseMathExpression();

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

std::unique_ptr<Expr> Parser::parseMathExpression()
{
    auto left = parseTerm();
    
    // Livello più basso di precedenza matematica [+ , -]
    while(check(TokenType::Plus) || check(TokenType::Minus))
    {
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
    while(check(TokenType::Star) || check(TokenType::Slash))
    {
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
        std::string lexeme = advance().lexeme;

        auto strExpr = std::make_unique<StringExpr>();
        strExpr->value = lexeme;
        return strExpr;
    }

    // Char letterali
    else if(check(TokenType::CharLiteral))
    {
        std::string ch_str = advance().lexeme; //stringa con un carattere

        auto charExpr = std::make_unique<CharExpr>();
        charExpr->value = ch_str.at(0); //estrazione char
        return charExpr;
    }

    // Booleano Letterale
    else if(check(TokenType::BoolLiteral))
    {
        std::string lexeme = advance().lexeme; //consuma il token valore
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

    // Chiamata a funzione
    else if(check(TokenType::Identifier) && peek(1).type == TokenType::LParen)
    {
        auto call = std::make_unique<CallExpr>();

        call->name = advance().lexeme;
        expect(TokenType::LParen);

        std::vector<std::unique_ptr<Expr>> args;

        while(!check(TokenType::RParen) && !isAtEnd())
        {
            args.push_back(parseExpr());

            if(check(TokenType::Comma))
                advance(); // consuma ',' se c'è un altro argomento
        }
        expect(TokenType::RParen);

        call->args = std::move(args);
        return call;
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

        auto expr = parseExpr(); //call ricorsiva
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
        errorLog->addError("Parse error. Expected a factor, received a different token. token: " +
                               typeToString(tokens.at(currentPos).type), peek().position);
        advance(); //error recovery, salta il token errato
        auto error = std::make_unique<ErrorExpr>();
        return error;
    }

    return nullptr;
}