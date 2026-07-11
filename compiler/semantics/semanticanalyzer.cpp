#include "semanticanalyzer.h"

SemanticAnalyzer::SemanticAnalyzer()
{
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

    for(const auto& st : program.statements) {
        analyzeStmt(st.get());
    }

    //placeholder
    for(const std::unique_ptr<Stmt>& st : program.statements) {
        if(!dynamic_cast<const FunctionStmt*>(st.get())) {
            this->errorLog->addError("presenza di stmt fuori da una funzione [alert placeholder]");
        }
    }
/*
 *  TODO !
    for(const std::unique_ptr<Stmt>& st : program.statements) {
        if(!dynamic_cast<const DeclarationStmt*>(st.get()) &&
            !dynamic_cast<const FunctionStmt*>(st.get()))
        {
            this->errorLog->addError("statement invalido come esterno ad una funzione");
        }
    }

    ELSE -> in ogni caso di analyzestmt, es assignment stmt:
        if(currentFunction == nullptr) {
            errorLog->addError("assegnazione invalida esterna ad una funzione");
        }
    più debole come strategia ma con messaggi di errore utili e più controllo sulla casistica.
*/
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

            if (!Type::isAssignmentCompatible(varType, valueResult.value_type)) {
                errorLog->addError("tipo incompatibile nell'assegnazione a " + s->name + "  " +
                                   "[confronto tra " + Type::toString(varType) + " e " + Type::toString(valueResult.value_type) + "]");
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

            if (!Type::isAssignmentCompatible(s->type, initResult.value_type)) {
                errorLog->addError("tipo incompatibile nell'inizializzazione di " + s->name + "  " +
                                   "[confronto tra " + Type::toString(s->type) + " e " + Type::toString(initResult.value_type) + "]");
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

            if(!Type::isAssignmentCompatible(currentFunction->returnType, res.value_type)) {
                errorLog->addError("could not convert " + Type::toString(currentFunction->returnType) +
                                   " to " + Type::toString(res.value_type) + " in return");
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
            return ExprAnalysisResult{ValueType::Error};
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

            if(!Type::isAssignmentCompatible(functionTable[s->name].paramTypes.at(i), res.value_type)) {
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
        ExprAnalysisResult operandResult = analyzeExpr(s->operand.get());

        ValueType resultType = Type::unaryResultType(s->op, operandResult.value_type);

        if (resultType == ValueType::Error && operandResult.value_type != ValueType::Error) {
            errorLog->addError("operatore unario non valido per il tipo " +
                               Type::toString(operandResult.value_type));
        }

        return ExprAnalysisResult{resultType};
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
 * Gli operatori binari devono controllare il tipo dei due operandi
 * prima di eseguire l'operazione, ogni operatore accetta tipi di operandi diversi.
 * Alcuni operatori permettono conversioni implicite del tipo di ritorno per permettere
 * l'operazione assegnata (promozione automatica).
 */

ExprAnalysisResult SemanticAnalyzer::analyzeBinaryOperation(const BinaryExpr *expr)
{
    ValueType leftType = analyzeExpr(expr->left.get()).value_type;
    ValueType rightType = analyzeExpr(expr->right.get()).value_type;

    ValueType resultType = Type::binaryResultType(expr->op, leftType, rightType);

    if(resultType == ValueType::Error) {
        errorLog->addError("operazione non valida tra tipi " +
                Type::toString(leftType) + " e " + Type::toString(rightType));

        return ExprAnalysisResult{ValueType::Error};
    }

    return ExprAnalysisResult{resultType};
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



