#include "semanticanalyzer.h"

#include <unordered_map>
#include <string>

#include "toplevel-rules.h"

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

    this->scopeStack.push_back(std::unordered_map<std::string, SymbolInfo>()); //scope globale

    //flaga di controllo
    bool winmain_found = false;
    bool valid = true;

    //controllo degli stmt top-level
    for(const std::unique_ptr<Stmt>& st : program.statements) 
    {
        if(isWinMain(st.get())) {
            winmain_found = true;
        }

        if(!isValidAtTopLevel(*st.get())) {
            this->errorLog->addError("invalid statement at top-level context. [invalidGlobalStmt]");
            valid = false;
        }
    }

    if(!winmain_found) {
        this->errorLog->addError("could not find winmain entrance for program");
        return;
    }

    if(!valid) return;

    //analisi del programma
    for(const auto& st : program.statements) {
        analyzeStmt(st.get());
    }

}

/*
 * Analizza lo statement fornito come parametro.
 * Tramite dynamic_cast, risale al tipo di statement fornito e lo instrada di conseguenza all'analisi
 * seguente, dopo aver gestito lo stato della tabella dei simboli se necessario.
 * Ogni caso richiama una funzione helper che contiene la logica di analisi di quel preciso stmt.
 */

void SemanticAnalyzer::analyzeStmt(const Stmt *stmt)
{
    // Nuovo Scope
    if(auto s = dynamic_cast<const BlockStmt*>(stmt))
    {
        analyzeBlockStmt(s);
    }

    // Assegnazione
    else if(auto s = dynamic_cast<const AssignmentStmt*>(stmt))
    {
        analyzeAssignment(s);
    }

    // Espressione
    else if(auto s = dynamic_cast<const ExpressionStmt*>(stmt))
    {
        analyzeExpr(s->expr.get());
    }

    // Dichiarazione
    else if(auto s = dynamic_cast<const DeclarationStmt*>(stmt))
    {
        analyzeDeclaration(s);
    }

    // Function Declaration
    else if(auto s = dynamic_cast<const FunctionStmt*>(stmt))
    {
        analyseFunction(s);
    }

    // Return Stmt
    else if(auto s = dynamic_cast<const ReturnStmt*>(stmt))
    {
        analyzeReturn(s);
    }

    // If Condition
    else if(auto s = dynamic_cast<const IfStmt*>(stmt))
    {
        analyzeIf(s);
    }

    // For Loop
    else if(auto s = dynamic_cast<const ForStmt*>(stmt))
    {
        analyzeFor(s);
    }

    // While Loop
    else if(auto s = dynamic_cast<const WhileStmt*>(stmt))
    {
        analyzeWhile(s);
    }

    // Switch
    else if(auto s = dynamic_cast<const SwitchStmt*>(stmt))
    {
        analyzeSwitch(s);
    }

    // Case
    else if(auto s = dynamic_cast<const CaseStmt*>(stmt))
    {
        // l'analisi di un case può trovarsi solo dentro uno switch stmt
        errorLog->addError("invalid case outside of switch");
    }

    // Default
    else if(auto s = dynamic_cast<const DefaultStmt*>(stmt))
    {
        // l'analisi di un default può trovarsi solo dentro uno switch stmt
        errorLog->addError("invalid default outside of switch");
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

void SemanticAnalyzer::analyzeBlockStmt(const BlockStmt* block) 
{
    pushScope(); //crea un nuovo scope

    for(const auto& st : block->statements) {
        analyzeStmt(st.get());
    }

    popScope(); //chiude lo scope corrente
}

void SemanticAnalyzer::analyzeAssignment(const AssignmentStmt* s) 
{
    ExprAnalysisResult targetResult = analyzeExpr(s->target.get());
    ExprAnalysisResult valueResult = analyzeExpr(s->value.get());

    if (!types::isAssignmentCompatible(targetResult.type, valueResult.type)) {
        errorLog->addError("tipo incompatibile nell'assegnazione a " + s->target_name + "  " +
                           "[confronto tra " + types::toString(targetResult.type) + " e " 
                           + types::toString(valueResult.type) + "]");
        return;
    }

    if(auto varExpr = dynamic_cast<const VariableExpr*>(s->target.get()))
    {
        if(symbolExistsAnywhere(varExpr->name) && targetResult.isConst) {
            errorLog->addError("forbidden assignment of const variable '" + varExpr->name + "'");
            return;
        }
    }
}

void SemanticAnalyzer::analyzeDeclaration(const DeclarationStmt* s)
{
    if(s->type.is(PrimitiveType::Void)) { 
        errorLog->addError("variable " + s->name + " declared void");
        return;
    }

    if(s->isConst && !s->initializer) {
        errorLog->addError("could not declare a const variable without initialization");
        declareSymbol(s->name, SymbolInfo(s->type, false)); // dichiarato ma non const, per evitare errori a cascata
        return;
    }

    if(s->initializer)
    {
        //risultato dell'espressione in assegnazione, valore che si sta assegnando
        ExprAnalysisResult initResult = analyzeExpr(s->initializer.get());

        if (!types::isAssignmentCompatible(s->type, initResult.type)) {
            errorLog->addError("tipo incompatibile nell'inizializzazione di " + s->name + "  " +
                                "[confronto tra " + types::toString(s->type) + " e " 
                                + types::toString(initResult.type) + "]");
        }
    }

    if(symbolExistsInCurrentScope(s->name)) {
        errorLog->addError("redeclaration of variable: " + s->name);
    } else {
        declareSymbol(s->name, SymbolInfo(s->type, s->isConst));
    }
}

void SemanticAnalyzer::analyseFunction(const FunctionStmt* s)
{
    if(functionTable.contains(s->name)) {
        // funzione già dichiarata
        errorLog->addError("redeclaration of function:" + s->name);
        return;
    }

    //raccolta dei type dei parametri
    std::vector<Type> paramsType;

    for(const FunctionParam& p : s->params) {
        paramsType.push_back(p.type);
    }

    functionTable.insert({s->name, FunctionInfo{s->returnType, paramsType}});

    //scope locale alla funzione
    pushScope();

    // dichiarazione dei parametri come variabili nello scope
    for(const FunctionParam& p : s->params) {
        declareSymbol(p.name, SymbolInfo(p.type, p.isConst));
    }

    currentFunction = &functionTable[s->name];

    // ogni funzione non-void deve avere un return valido per ogni path
    if(!s->returnType.is(PrimitiveType::Void) && !allPathsReturn(s->body.get())) {
        errorLog->addError("not all code paths return a value in function " + s->name);
    }

    analyzeStmt(s->body.get());

    popScope();

    currentFunction = nullptr;
}

void SemanticAnalyzer::analyzeReturn(const ReturnStmt* s)
{
    if(currentFunction == nullptr) {
        errorLog->addError("return stmt fuori da una funzione");
        return;
    }

    if(currentFunction->returnType.is(PrimitiveType::Void) && s->value != nullptr) {
        errorLog->addError("returning a value in a function declared void");
        return;
    }

    if(!currentFunction->returnType.is(PrimitiveType::Void) && s->value == nullptr) {
        errorLog->addError("return stmt with no value in a function returning non-void");
        return;
    }

    if(s->value != nullptr) {
        // controllo del tipo dell'espressione (return expr;)
        ExprAnalysisResult res = analyzeExpr(s->value.get());

        if(!types::isAssignmentCompatible(currentFunction->returnType, res.type)) {
            errorLog->addError("could not convert " + types::toString(currentFunction->returnType) +
                               " to " + types::toString(res.type) + " in return");
            return;
        }
    }
}

void SemanticAnalyzer::analyzeIf(const IfStmt* s)
{
    ExprAnalysisResult condResult = analyzeExpr(s->condition.get());

    if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
        errorLog->addError("if condition must be of type boolean");
    }

    analyzeStmt(s->thenBranch.get()); //il body è uno stmt

    if(s->elseBranch) {
        analyzeStmt(s->elseBranch.get()); // se != nullptr
    }
}

void SemanticAnalyzer::analyzeFor(const ForStmt* s)
{
    pushScope(); // scope che racchiude init, condition, update, body

    if(s->init) analyzeStmt(s->init.get());
    if(s->condition) {
        ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
        if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
            errorLog->addError("la condizione del for deve essere di tipo bool");
        }
    }
    if(s->update) analyzeExpr(s->update.get());

    loopDepth++;
    analyzeStmt(s->body.get());
    loopDepth--;

    popScope();
}

void SemanticAnalyzer::analyzeWhile(const WhileStmt* s)
{
    ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
    if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
        errorLog->addError("la condizione del while deve essere di tipo bool");
    }

    loopDepth++;
    analyzeStmt(s->body.get());
    loopDepth--;
}

void SemanticAnalyzer::analyzeSwitch(const SwitchStmt* s)
{
    ExprAnalysisResult scrutineeResult = analyzeExpr(s->scrutinee.get());
    
    // controllo di validità del tipo dello scrutinee
    switch (scrutineeResult.type.asPrimitive())
    {
    case PrimitiveType::Int:
    case PrimitiveType::Char:
        break;
    
    default:
        errorLog->addError("invalid type in switch condition (" 
            + types::toString(scrutineeResult.type) + ")");
        break;
    }

    for(const auto& c : s->cases) {
        analyzeCase(c.get(), scrutineeResult.type.asPrimitive());
    }

    if(s->_default) {
        analyzeDefault(s->_default.get());
    }
}

void SemanticAnalyzer::analyzeCase(const CaseStmt* s, const PrimitiveType& switch_type) 
{
    ExprAnalysisResult condResult = analyzeExpr(s->label.get());

    if(condResult.type.asPrimitive() != switch_type) {
        //non considera nessuna promozione automatica
        errorLog->addError("case label value is incompatible with switch value");
        return;
    }

    //ogni label deve essere un valore costante a compile time
    if(dynamic_cast<const CharExpr*>(s->label.get()) || dynamic_cast<const NumberExpr*>(s->label.get())) {
        // il valore è di natura costante
    } else {
        errorLog->addError("case label value must be constant and known at compile-time");
    }

    for(const auto& st : s->body) {
        //un case stmt va instradato direttamente
        if(auto d = dynamic_cast<const CaseStmt*>(st.get())) {
            analyzeCase(d, switch_type);
        } else {
            analyzeStmt(st.get());
        }
    }

}

void SemanticAnalyzer::analyzeDefault(const DefaultStmt* s)
{
    for(const auto& st : s->body) {
        analyzeStmt(st.get());
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
        result.type = s->isInteger ? Type(PrimitiveType::Int) : Type(PrimitiveType::Double);
        return result;
    }

    // String Expression
    else if(auto s = dynamic_cast<const StringExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.type = Type(PrimitiveType::String);
        return result;
    }

    // Char Expression
    else if(auto s = dynamic_cast<const CharExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.type = Type(PrimitiveType::Char);
        return result;
    }

    // Boolean Expression
    else if(auto s = dynamic_cast<const BooleanExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.type = Type(PrimitiveType::Bool);
        return result;
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        if(!symbolExistsAnywhere(s->name)) {
            errorLog->addError("variable not defined. variable name: " + s->name);
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        }
        ExprAnalysisResult result;
        result.type = lookupSymbolInfo(s->name).type;
        result.isConst = lookupSymbolInfo(s->name).isConst;
        return result;
    }

    //Array literal Expression
    else if(auto s = dynamic_cast<const LiteralArrayExpr*>(expr))
    {
        if(s->elements.empty()) {
            errorLog->addError("could not convert empty enclosed-bracket to an array");
            return ExprAnalysisResult{Type(PrimitiveType::Error)};
        }

        auto arrayType = analyzeExpr(s->elements.at(0).get()).type;

        for(int i=1; i < s->elements.size(); ++i)
        {
            auto type = analyzeExpr(s->elements.at(i).get()).type;

            if(type.asPrimitive() != arrayType.asPrimitive()) {
                errorLog->addError("incompatible element of type " + types::toString(type) +
                                    " in literal array of type " + types::toString(arrayType) + 
                                    " at index " + std::to_string(i));
                return ExprAnalysisResult{Type(PrimitiveType::Error)};
            }
        }

        auto elementType = arrayType.asPrimitive();
        ArrayType arr(elementType, s->elements.size());

        //salva il valore ricostruito nell'espressione
        s->type = arr; 

        return ExprAnalysisResult{Type{arr}};
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {
        if(!functionTable.contains(s->name)) {
            errorLog->addError(s->name + " was not declared in this scope");
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        }

        if(s->args.size() != functionTable[s->name].paramTypes.size()) {
            errorLog->addError("errore _#325 - callexpr in analyseExpr");
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        }

        for(int i=0; i < s->args.size(); ++i) {
            auto res = analyzeExpr(s->args.at(i).get());

            if(!types::isAssignmentCompatible(functionTable[s->name].paramTypes.at(i), res.type)) {
                errorLog->addError("error _#332 - callexpr in analyze expr");
                return ExprAnalysisResult(Type(PrimitiveType::Error));
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

        Type resultType = Type(types::unaryResultType(s->op, operandResult.type.asPrimitive()));

        if (resultType.is(PrimitiveType::Error) && !operandResult.type.is(PrimitiveType::Error)) {
            errorLog->addError("operatore unario non valido per il tipo " +
                               types::toString(operandResult.type));
        }

        return ExprAnalysisResult{resultType};
    }

    // Error Expression
    else if(auto s = dynamic_cast<const ErrorExpr*>(expr))
    {
        ExprAnalysisResult result;
        result.type = Type(PrimitiveType::Error);
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
    Type leftType = analyzeExpr(expr->left.get()).type;
    Type rightType = analyzeExpr(expr->right.get()).type;

    PrimitiveType resultType = types::binaryResultType(expr->op, leftType.asPrimitive(), rightType.asPrimitive());

    if(resultType == PrimitiveType::Error) {
        errorLog->addError("operazione non valida tra tipi " +
                types::toString(leftType) + " e " + types::toString(rightType));

        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }

    return ExprAnalysisResult(Type(resultType));
}

/*
 * Funzione dello stackScope.
 * Controlla l'esistenza di un simbolo all'interno di tutto lo stack degli scope
 * presenti.
 */

bool SemanticAnalyzer::symbolExistsAnywhere(const std::string &name) const {
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

bool SemanticAnalyzer::symbolExistsInCurrentScope(const std::string &name) const {
    if(scopeStack.back().contains(name))
        return true;
    else
        return false;
}

/*
 * Funzione dello stackScope.
 * Cerca il symbolo specificato in tutto lo stackscope, per poi restituire
 * le informazioni di quel simbolo.
 */

SymbolInfo SemanticAnalyzer::lookupSymbolInfo(const std::string &name) const {
    for(int i = scopeStack.size() - 1; i >= 0; i--) {
        if (scopeStack[i].contains(name)) return scopeStack[i].at(name);
    }
    return SymbolInfo(Type(PrimitiveType::Error), false); // non trovato
}

/*
 * Funzione dello stackScope.
 * Permette di dichiarare un simbolo, inserendolo nello scope corrente, che
 * corrisponde all'ultimo della lista.
 */

void SemanticAnalyzer::declareSymbol(const std::string &name, SymbolInfo info) {
    scopeStack.back().insert({name, info});
}

/*
 * Funzione dello stackScope.
 * Esegue il push sullo stackscope, ovvero aggiunge un nuovo 
 * scope (std::unordered_map<std::string, SymbolInfo) all'interno della lista.
 * Lo scope aggiunto è una mappa vuota.
 */

void SemanticAnalyzer::pushScope() {
    scopeStack.push_back(std::unordered_map<std::string, SymbolInfo>());
}

/*
 * Funzione dello stackScope.
 * Simmetrico all'azione di push, esegue l'azione di pop, ovvero esce dallo scope attuale.
 */

void SemanticAnalyzer::popScope() {
    scopeStack.pop_back();
}

/*
 * Questa funzione controlla ricorsivamente che ogni percorso possibile di uno stmt
 * finisca in qualche modo con un ritorno valido.
 */

bool SemanticAnalyzer::allPathsReturn(const Stmt* stmt) const
{
    if(auto d = dynamic_cast<const ReturnStmt*>(stmt)) {
        return true;
    }
    else if(auto d = dynamic_cast<const BlockStmt*>(stmt)) {
        for(const auto& st : d->statements) {
            if(allPathsReturn(st.get())) return true;
        }
    }
    else if(auto d = dynamic_cast<const IfStmt*>(stmt)) {
        bool thenB = allPathsReturn(d->thenBranch.get());
        bool elseB = allPathsReturn(d->elseBranch.get());

        if(thenB && elseB) return true;
    }

    return false;
}



