#include "semanticanalyzer.h"

SemanticAnalyzer::SemanticAnalyzer()
{
}

/*
 * Questa funzione controlla se un'operazione di assegnazione è fattibile in base ai tipi degli
 * operandi.
 * I due parametri rappresentano il tipo della variabile a cui si assegna il valore (variableType) ed
 * il tipo del valore che si sta assegnando (assignType).
 * es: int x = 5.0; (variableType è int mentre assignType è double).
 */

bool SemanticAnalyzer::isAssignmentCompatible(ValueType variableType, ValueType assignType) {
    if (assignType == ValueType::Error) return true;  // errore già segnalato altrove, non duplicare

    if (variableType == assignType) return true;

    // promozione Int -> Double
    if (variableType == ValueType::Double && assignType == ValueType::Int) return true;

    return false;
}

/*
 * Punto di ingresso dell'analisi semantica del programma.
 * Itera su ogni statement del programma in questione, richiamando un'analisi su ognuno di essi.
 * Alla fine delle chiamate di funzioni di analisi, tutti gli errori sono stati elaborati.
 */

void SemanticAnalyzer::analyzeProgram(const Program &program, ErrorLog &errorLog)
{
    this->errorLog = &errorLog;
    this->scopeStack.clear();
    this->functionTable.clear();
    this->currentFunction = nullptr;
    this->loopDepth = 0;

    this->scopeStack.append(QMap<QString, SymbolInfo>()); //scope globale

    for(const auto& s : program.statements) {
        analyzeStmt(s.get());
    }

    qDebug() << "FUNCTION TABLE:";
    for(const auto& [key, a] : this->functionTable.asKeyValueRange()) {
        qDebug() << key;
    }
}

/*
 * Analizza lo statement fornito come parametro.
 * Tramite dynamic_cast, risale al tipo di statement fornito e lo instrada di conseguenza all'analisi
 * seguente, dopo aver gestito lo stato della tabella dei simboli se necessario.
 */

void SemanticAnalyzer::analyzeStmt(const Stmt *stmt)
{
    // Nuovo Scope
    if(auto s = dynamic_cast<const BlockStmt*>(stmt))
    {
        pushScope(); //crea un nuovo scope

        for(const auto& st : s->statements) {
            analyzeStmt(st.get());
        }

        popScope(); //chiude lo scope corrente
    }

    // Assegnazione
    else if(auto s = dynamic_cast<const AssignmentStmt*>(stmt))
    {
        // risultato dell'espressione di assegnazione, valore che si sta assegnando
        ExprAnalysisResult valueResult = analyzeExpr(s->value.get());

        if (symbolExistsAnywhere(s->name))
        {
            /* Controllo di tipo nell'operatore di assegnazione */
            ValueType varType = lookupSymbolInfo(s->name).type;

            if (!isAssignmentCompatible(varType, valueResult.value_type)) {
                errorLog->addError("tipo incompatibile nell'assegnazione a " + s->name + "  " +
                                   "[confronto tra " + toString(varType) + " e " + toString(valueResult.value_type) + "]");
            }
        }
        else {
            errorLog->addError("variabile non dichiarata: " + s->name);
        }
    }

    // Espressione
    else if(auto s = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        analyzeExpr(s->expr.get());
    }

    // Dichiarazione
    else if(auto s = dynamic_cast<const DeclarationStmt*>(stmt))
    {
        if(s->initializer)
        {
            //risultato dell'espressione in assegnazione, valore che si sta assegnando
            ExprAnalysisResult initResult = analyzeExpr(s->initializer.get());

            if(s->type == ValueType::Void) {
                errorLog->addError("variable declared void");
                return;
            }

            if (!isAssignmentCompatible(s->type, initResult.value_type)) {
                errorLog->addError("tipo incompatibile nell'inizializzazione di " + s->name + "  " +
                                   "[confronto tra " + toString(s->type) + " e " + toString(initResult.value_type) + "]");
            }
        }

        if (symbolExistsInCurrentScope(s->name)) {
            errorLog->addError("redeclaration of variable: " + s->name);
        } else {
            declareSymbol(s->name, SymbolInfo{s->type});
        }
    }

    // Function Declaration
    else if(auto s = dynamic_cast<const FunctionStmt*>(stmt))
    {
        if(functionTable.contains(s->name)) {
            // funzione già dichiarata
            errorLog->addError("redeclaration of function:" + s->name);
            return;
        }

        // --- raccolta dei tipi dei parametri --- //
        std::vector<ValueType> paramsType;

        for(const FunctionParam& p : s->params) {
            paramsType.push_back(p.type);
        }

        // --- insert nella tabella --- //
        functionTable.insert(s->name, FunctionInfo{s->returnType, paramsType});

        // --- analisi del codice della funzione --- //
        pushScope();

        // dichiarazione dei parametri come variabili nello scope
        for(const FunctionParam& p : s->params) {
            declareSymbol(p.name, SymbolInfo{p.type});
        }

        currentFunction = &functionTable[s->name];

        analyzeStmt(s->body.get());

        popScope();

        currentFunction = nullptr;
    }

    // Return Stmt
    else if(auto s = dynamic_cast<const ReturnStmt*>(stmt))
    {
        if(currentFunction == nullptr) {
            errorLog->addError("return stmt fuori da una funzione");
            return;
        }

        if(currentFunction->returnType == ValueType::Void && s->value != nullptr) {
            errorLog->addError("returning a value in a function declared void");
            return;
        }

        if(currentFunction->returnType != ValueType::Void && s->value == nullptr) {
            errorLog->addError("return stmt with no value in a function returning non-void");
            return;
        }

        if(s->value != nullptr) {
            // controllo del tipo dell'espressione (return expr;)
            ExprAnalysisResult res = analyzeExpr(s->value.get());

            if(!isAssignmentCompatible(currentFunction->returnType, res.value_type)) {
                errorLog->addError("could not convert " + toString(currentFunction->returnType) +
                                   " to " + toString(res.value_type) + " in return");
                return;
            }
        }
    }

    // If Condition
    else if(auto s = dynamic_cast<const IfStmt*>(stmt))
    {
        ExprAnalysisResult condResult = analyzeExpr(s->condition.get());

        if(condResult.value_type != ValueType::Bool && condResult.value_type != ValueType::Error) {
            errorLog->addError("if condition must be of type boolean");
        }

        analyzeStmt(s->thenBranch.get()); //il body è uno stmt

        if(s->elseBranch) {
            analyzeStmt(s->elseBranch.get()); // se != nullptr
        }
    }

    // For Loop
    else if(auto s = dynamic_cast<const ForStmt*>(stmt))
    {
        pushScope(); // scope che racchiude init, condition, update, body

        if(s->init) analyzeStmt(s->init.get());
        if(s->condition) {
            ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
            if(condResult.value_type != ValueType::Bool && condResult.value_type != ValueType::Error) {
                errorLog->addError("la condizione del for deve essere di tipo bool");
            }
        }
        if(s->update) analyzeExpr(s->update.get());

        loopDepth++;
        analyzeStmt(s->body.get());
        loopDepth--;

        popScope();
    }

    // While Loop
    else if(auto s = dynamic_cast<const WhileStmt*>(stmt))
    {
        ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
        if(condResult.value_type != ValueType::Bool && condResult.value_type != ValueType::Error) {
            errorLog->addError("la condizione del while deve essere di tipo bool");
        }

        loopDepth++;
        analyzeStmt(s->body.get());
        loopDepth--;
    }

    // Break Stmt
    else if(dynamic_cast<const BreakStmt*>(stmt))
    {
        if(loopDepth == 0) {
            errorLog->addError("break fuori da un ciclo");
        }
    }

    // continue Stmt
    else if(dynamic_cast<const ContinueStmt*>(stmt))
    {
        if(loopDepth == 0) {
            errorLog->addError("continue fuori da un ciclo");
        }
    }

    // Errore
    else if(auto s = dynamic_cast<const ErrorStmt*>(stmt))
    {
        //blocco vuoto, nessuna azione richiesta poichè l'errore è gia stato segnalato
    }
}

/*
 * Analizza l'espressione fornita come parametro.
 * Tramite dynamic_cast risale al tipo di espressione fornita, per poi analizzare la correttezza
 * semantica di ciascuna espressione di conseguenza.
 * Per ogni tipo di espressione, vengono svolti controlli differenti legati al tipo stesso.
 *
 * Ritorna una struttura modificabile nel tempo, che permette di passare tutte le informazioni
 * necessarie per l'analisi.
 */

ExprAnalysisResult SemanticAnalyzer::analyzeExpr(const Expr *expr)
{
    // Number Expression
    if(auto s = dynamic_cast<const NumberExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.value_type = s->isInteger ? ValueType::Int : ValueType::Double;
        return result;
    }

    // String Expression
    else if(auto s = dynamic_cast<const StringExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.value_type = ValueType::String;
        return result;
    }

    // Char Expression
    else if(auto s = dynamic_cast<const CharExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.value_type = ValueType::Char;
        return result;
    }

    // Boolean Expression
    else if(auto s = dynamic_cast<const BooleanExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.value_type = ValueType::Bool;
        return result;
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        if(!symbolExistsAnywhere(s->name)) {
            errorLog->addError("variable not defined. variable name: " + s->name);
        }
        ExprAnalysisResult result;
        result.value_type = lookupSymbolInfo(s->name).type;
        return result;
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {
        if(!functionTable.contains(s->name)) {
            errorLog->addError(s->name + "was not declared in this scope");
            return ExprAnalysisResult{ValueType::Error};
        }

        if(s->args.size() != functionTable[s->name].paramTypes.size()) {
            errorLog->addError("errore #325 - callexpr in analyseExpr");
            return ExprAnalysisResult{ValueType::Error};
        }

        for(int i=0; i < s->args.size(); ++i) {
            auto res = analyzeExpr(s->args.at(i).get());

            if(!isAssignmentCompatible(res.value_type, functionTable[s->name].paramTypes.at(i))) {//ordine parametri giusto??
                errorLog->addError("error#332 - callexpr in analyze expr");
                return ExprAnalysisResult{ValueType::Error};
            }
        }

        return ExprAnalysisResult{functionTable[s->name].returnType};
    }

    // Binary Expression
    else if(auto s = dynamic_cast<const BinaryExpr*>(expr))
    {
        return analyzeBinaryOperation(s);
    }

    // Unary Expression
    else if(auto s = dynamic_cast<const UnaryExpr*>(expr))
    {
        //recursive call
        return analyzeExpr(s->operand.get());
    }

    // Error Expression
    else if(auto s = dynamic_cast<const ErrorExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.value_type = ValueType::Error;
        return result;
    }

    return ExprAnalysisResult{};
}

/*
 * Controllo bool del tipo di un'espressione
 */
bool SemanticAnalyzer::isNumeric(ValueType t) {
    return t == ValueType::Int || t == ValueType::Double;
}

/*
 * Gli operatori binari devono controllare il tipo dei due operandi
 * prima di eseguire l'operazione, ogni operatore accetta tipi di operandi diversi.
 * Alcuni operatori permettono conversioni implicite del tipo di ritorno per permettere
 * l'operazione assegnata (promozione automatica).
 */

ExprAnalysisResult SemanticAnalyzer::analyzeBinaryOperation(const BinaryExpr *expr)
{
    ExprAnalysisResult result;

    // Controllo dei tipi degli operandi
    ValueType leftType = analyzeExpr(expr->left.get()).value_type;
    ValueType rightType = analyzeExpr(expr->right.get()).value_type;

    bool leftIsTextual = (leftType == ValueType::String || leftType == ValueType::Char);
    bool rightIsTextual = (rightType == ValueType::String || rightType == ValueType::Char);

    bool bothNumeric = isNumeric(leftType) && isNumeric(rightType);
    bool bothChar = (leftType == ValueType::Char && rightType == ValueType::Char);
    bool bothString = (leftType == ValueType::String && rightType == ValueType::String);

    //se si trova un errore l'espressione viene scartata come errore
    if(leftType == ValueType::Error || rightType == ValueType::Error) {
        result.value_type = ValueType::Error;
        return result;
    }

    /* Controllo dei tipi di ritorno */

    // Uguaglianza / Disuguaglianza : operatori [ ==, != ]
    if (expr->op == TokenType::EqualEqual || expr->op == TokenType::NotEqual)
    {
        if (bothNumeric || bothChar || bothString) {
            result.value_type = ValueType::Bool;
        } else {
            errorLog->addError("operatore di uguaglianza non valido tra tipi " + toString(leftType) + " e " + toString(rightType));
            result.value_type = ValueType::Error;
        }
    }

    // Confronti Relazionali : operatori [ >, <, >=, <= ]
    else if (expr->op == TokenType::Less || expr->op == TokenType::Greater ||
             expr->op == TokenType::LessEqual || expr->op == TokenType::GreaterEqual)
    {
        if (bothNumeric || bothChar || bothString) {
            result.value_type = ValueType::Bool;
        } else {
            errorLog->addError("operatore di confronto non valido tra tipi " + toString(leftType) + " e " + toString(rightType));
            result.value_type = ValueType::Error;
        }
    }

    // Operatori Logici : operatori [ &&, !, || ]
    else if (expr->op == TokenType::LogicalAnd || expr->op == TokenType::LogicalOr)
    {
        if (leftType == ValueType::Bool && rightType == ValueType::Bool) {
            result.value_type = ValueType::Bool;
        } else {
            errorLog->addError("operatore logico non valido tra tipi " + toString(leftType) + " e " + toString(rightType));
            result.value_type = ValueType::Error;
        }
    }

    // Somma Aritmetica : operatore [ + ]
    else if(expr->op == TokenType::Plus)
    {
        if (isNumeric(leftType) && isNumeric(rightType))
        {
            if (leftType == ValueType::Double || rightType == ValueType::Double)
                result.value_type = ValueType::Double;  // promozione se almeno uno è Double
            else
                result.value_type = ValueType::Int;     // entrambi Int
        }
        else if (leftIsTextual && rightIsTextual) {
            result.value_type = ValueType::String;  // concatenazione, sempre risultato string anche se input erano char
        }
        else {
            errorLog->addError("operatore + non valido tra tipi " + toString(leftType) + " e " + toString(rightType));
            result.value_type = ValueType::Error;
        }
    }

    // Minus, Star, Slash : operatori [ -, *, / ]
    else
    {
        if (isNumeric(leftType) && isNumeric(rightType))
        {
            if (leftType == ValueType::Double || rightType == ValueType::Double)
                result.value_type = ValueType::Double;  // promozione se almeno uno è Double
            else
                result.value_type = ValueType::Int;     // entrambi Int
        }
        else {
            errorLog->addError("operatore aritmetico non valido tra tipi " + toString(leftType) + " e " + toString(rightType));
            result.value_type = ValueType::Error;
        }
    }

    return result;
}

/*
 * Funzione dello stackScope.
 * Controlla l'esistenza di un simbolo all'interno di tutto lo stack degli scope
 * presenti.
 */

bool SemanticAnalyzer::symbolExistsAnywhere(const QString &name) const {
    for(int i= scopeStack.size() -1; i >= 0; i--) {
        if(scopeStack[i].contains(name)) return true;
    }
    return false;
}

/*
 * Funzione dello stackScope.
 * Controlla l'esistenza di un simbolo solo nello scope corrente, che
 * corrisponde all'ultimo scope della lista.
 */

bool SemanticAnalyzer::symbolExistsInCurrentScope(const QString &name) const {
    if(scopeStack.last().contains(name))
        return true;
    else
        return false;
}

/*
 * Funzione dello stackScope.
 * Cerca il symbolo specificato in tutto lo stackscope, per poi restituire
 * le informazioni di quel simbolo.
 */

SymbolInfo SemanticAnalyzer::lookupSymbolInfo(const QString &name) const {
    for(int i = scopeStack.size() - 1; i >= 0; i--) {
        if (scopeStack[i].contains(name)) return scopeStack[i].value(name);
    }
    return SymbolInfo{ValueType::Error}; // non trovato
}

/*
 * Funzione dello stackScope.
 * Permette di dichiarare un simbolo, inserendolo nello scope corrente, che
 * corrisponde all'ultimo della lista.
 */

void SemanticAnalyzer::declareSymbol(const QString &name, SymbolInfo info) {
    scopeStack.last().insert(name, info);
}

/*
 * Funzione dello stackScope.
 * Esegue il push sullo stackscope, ovvero aggiunge un nuovo scope (QMap<QString, SymbolInfo)
 * all'interno della lista.
 * Lo scope aggiunto è una mappa vuota.
 */

void SemanticAnalyzer::pushScope() {
    scopeStack.append(QMap<QString, SymbolInfo>());
}

/*
 * Funzione dello stackScope.
 * Simmetrico all'azione di push, esegue l'azione di pop, ovvero esce dallo scope attuale.
 */

void SemanticAnalyzer::popScope() {
    scopeStack.removeLast();
}



