#include "semanticanalyzer.h"

#include <unordered_map>
#include <string>
#include <variant>

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
    this->currentFunction = nullptr;
    this->loopDepth = 0;

    this->scopeStack.push(); //scope globale

    //flag di controllo
    bool winmain_found = false;
    bool valid = true;

    //controllo degli stmt top-level
    for(const std::unique_ptr<Stmt>& st : program.statements) 
    {
        if(isWinMain(st.get())) {
            winmain_found = true;
        }

        if(!isValidAtTopLevel(*st.get())) {
            this->errorLog->addError("invalid statement at top-level context. [invalidGlobalStmt]", st->position);
            valid = false;
        }
    }

    if(!winmain_found) {
        this->errorLog->addError("could not find winmain entrance for program", program.statements.at(0)->position);
        return;
    }

    if(!valid) return;

    //analisi del programma
    for(const auto& st : program.statements) {
        analyzeStmt(st.get());
    }

}

void SemanticAnalyzer::assignNamespaceTable(NamespaceTable& namespaceTable)
{
    this->namespaceTable = &namespaceTable;
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
        errorLog->addError("invalid case outside of switch", s->position);
    }

    // Default
    else if(auto s = dynamic_cast<const DefaultStmt*>(stmt))
    {
        // l'analisi di un default può trovarsi solo dentro uno switch stmt
        errorLog->addError("invalid default outside of switch", s->position);
    }

    // Break Stmt
    else if(auto s = dynamic_cast<const BreakStmt*>(stmt))
    {
        if(loopDepth == 0) {
            errorLog->addError("invalid break stmt inside current scope [-InvalidBreakContext]", s->position);
        }
    }

    // continue Stmt
    else if(auto s = dynamic_cast<const ContinueStmt*>(stmt))
    {
        if(loopDepth == 0) {
            errorLog->addError("invalid continue stmt inside current scope [-InvalidContinueContext]", s->position);
        }
    }

    // Namespace
    else if(auto s = dynamic_cast<const NamespaceStmt*>(stmt))
    {
        analyzeNamespace(s);
    }

    // Errore
    else if(auto s = dynamic_cast<const ErrorStmt*>(stmt))
    {
        //blocco vuoto, nessuna azione richiesta poichè l'errore è gia stato segnalato
    }
}

void SemanticAnalyzer::analyzeBlockStmt(const BlockStmt* block) 
{
    scopeStack.push(); //crea un nuovo scope

    for(const auto& st : block->statements) {
        analyzeStmt(st.get());
    }

    scopeStack.pop(); //chiude lo scope corrente
}

void SemanticAnalyzer::analyzeDeclaration(const DeclarationStmt* s)
{
    if(s->type.is(PrimitiveType::Void)) { 
        errorLog->addError("variable " + s->name + " declared void", s->position);
        return;
    }

    if(s->isConst && !s->initializer) {
        errorLog->addError("could not declare a const variable without initialization", s->position);
        return;
    }

    if(s->initializer)
    {
        //risultato dell'espressione in assegnazione, valore che si sta assegnando
        ExprAnalysisResult initResult = analyzeExpr(s->initializer.get());

        if (!types::isAssignmentCompatible(s->type, initResult.type)) {
            errorLog->addError("tipo incompatibile nell'inizializzazione di " + s->name + "  " +
                                "[confronto tra " + types::toString(s->type) + " e " 
                                + types::toString(initResult.type) + "]", s->position);
        }
    }

    if(scopeStack.isGlobalScope() && s->initializer) 
    {
        //dichiarazione globale, controllo di valore const
        auto* init = s->initializer.get();
        bool isLiteral = dynamic_cast<const NumberExpr*>(init)
                        || dynamic_cast<const CharExpr*>(init)
                        || dynamic_cast<const BooleanExpr*>(init);

        if(!isLiteral) {
            errorLog->addError("global variable '" + s->name + "' must be initialized with a constant literal", s->position);
            return;
        }
    }

    if(scopeStack.symbolExistsInCurrentScope(s->name)) {
        errorLog->addError("redeclaration of variable: " + s->name, s->position);
    } else {
        scopeStack.declareSymbol(s->name, Symbol(VariableSymbol(s->type, s->isConst)));
    }
}

void SemanticAnalyzer::analyseFunction(const FunctionStmt* s)
{
    if(currentFunction != nullptr) {
        errorLog->addError("could not declare a function inside another function scope", s->position);
        return;
    }

    if(scopeStack.symbolExistsAnywhere(s->name)) {
        // funzione già dichiarata
        errorLog->addError("redeclaration of function:" + s->name, s->position);
        return;
    }

    //raccolta dei type dei parametri
    std::vector<Type> paramsType;

    for(const FunctionParam& p : s->params) {
        paramsType.push_back(p.type);
    }

    FunctionSymbol func{s->returnType, paramsType};
    scopeStack.declareSymbol(s->name, Symbol(func));

    // prende per riferimento un elemento dalla tabella interna dello scopestack
    currentFunction = &std::get<FunctionSymbol>(scopeStack.getSymbolPtr(s->name)->category);

    //scope locale alla funzione
    scopeStack.push();

    // dichiarazione dei parametri come variabili nello scope
    for(const FunctionParam& p : s->params) {
        scopeStack.declareSymbol(p.name, Symbol(VariableSymbol(p.type, p.isConst)));
    }

    // ogni funzione non-void deve avere un return valido per ogni path
    if(!s->returnType.is(PrimitiveType::Void) && !allPathsReturn(s->body.get())) {
        errorLog->addError("not all code paths return a value in function " + s->name, s->position);
    }

    analyzeStmt(s->body.get());

    scopeStack.pop();

    currentFunction = nullptr;
}

void SemanticAnalyzer::analyzeReturn(const ReturnStmt* s)
{
    if(currentFunction == nullptr) {
        errorLog->addError("return stmt fuori da una funzione", s->position);
        return;
    } 

    if(currentFunction->returnType.is(PrimitiveType::Void) && s->value != nullptr) {
        errorLog->addError("returning a value in a function declared void", s->position);
        return;
    }

    if(!currentFunction->returnType.is(PrimitiveType::Void) && s->value == nullptr) {
        errorLog->addError("return stmt with no value in a function returning non-void", s->position);
        return;
    }

    if(s->value != nullptr) {
        // controllo del tipo dell'espressione (return expr;)
        ExprAnalysisResult res = analyzeExpr(s->value.get());

        if(!types::isAssignmentCompatible(currentFunction->returnType, res.type)) {
            errorLog->addError("could not convert " + types::toString(res.type) +
                               " to " + types::toString(currentFunction->returnType) + " in return", s->position);
            return;
        }
    }
}

void SemanticAnalyzer::analyzeIf(const IfStmt* s)
{
    ExprAnalysisResult condResult = analyzeExpr(s->condition.get());

    if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
        errorLog->addError("if condition must be of type boolean", s->position);
    }

    analyzeStmt(s->thenBranch.get()); //il body è uno stmt

    if(s->elseBranch) {
        analyzeStmt(s->elseBranch.get()); // se != nullptr
    }
}

void SemanticAnalyzer::analyzeFor(const ForStmt* s)
{
    scopeStack.push(); // scope che racchiude init, condition, update, body

    if(s->init) analyzeStmt(s->init.get());
    if(s->condition) {
        ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
        if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
            errorLog->addError("la condizione del for deve essere di tipo bool", s->position);
        }
    }
    if(s->update) analyzeExpr(s->update.get());

    loopDepth++;
    analyzeStmt(s->body.get());
    loopDepth--;

    scopeStack.pop();
}

void SemanticAnalyzer::analyzeWhile(const WhileStmt* s)
{
    ExprAnalysisResult condResult = analyzeExpr(s->condition.get());
    if(!condResult.type.is(PrimitiveType::Bool) && !condResult.type.is(PrimitiveType::Error)) {
        errorLog->addError("la condizione del while deve essere di tipo bool", s->position);
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
            + types::toString(scrutineeResult.type) + ")", s->position);
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
        errorLog->addError("case label value is incompatible with switch value", s->position);
        return;
    }

    //ogni label deve essere un valore costante a compile time
    if(dynamic_cast<const CharExpr*>(s->label.get()) || dynamic_cast<const NumberExpr*>(s->label.get())) {
        // il valore è di natura costante
    } else {
        errorLog->addError("case label value must be constant and known at compile-time", s->position);
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
 * Si occupa dell'analisi di una dichiarazione di  namespace.
 * All'interno di un namespace possono esistere solo dichiarazioni di simboli, quindi sono 
 * accettati e controllati solo symbols e non stmt di tipo generale.
 * 
 * nota: questa funzione si divide in due parti, la prima di "update" della namespace table, 
 * e la seconda di analisi effettiva, che richiama le stesse funzioni dell'analisi semantica
 * trattando il namespace come un nuovo scope.
 */

void SemanticAnalyzer::analyzeNamespace(const NamespaceStmt* s)
{
    namespaceTable->push(s->name);
    scopeStack.push(); //? l'analisi deve cmq trattare il namespace come scope

    for(const auto& st : s->body)
    {
        if(auto d = dynamic_cast<const DeclarationStmt*>(st.get()))
        {
            if(namespaceTable->symbolExistInCurrentNamespace(d->name)) {
                errorLog->addError("redeclaration of symbol: " + s->name + "::" + d->name, s->position);
                continue;
            } else {
                namespaceTable->declareSymbol(d->name, Symbol(VariableSymbol(d->type, d->isConst)));
            }

            analyzeStmt(d);
        }
        else if(auto d = dynamic_cast<const FunctionStmt*>(st.get()))
        {
            if(namespaceTable->symbolExistInCurrentNamespace(d->name)) {
                errorLog->addError("redeclaration of function: " + d->name, s->position);
                continue;
            } else {
                std::vector<Type> paramTypes;
                for(const auto p : d->params) {
                    paramTypes.push_back(p.type);
                }
                namespaceTable->declareSymbol(d->name, Symbol(FunctionSymbol(d->returnType, paramTypes)));
            }

            analyzeStmt(d);
        }
        else {
            errorLog->addError("invalid statement in namespace definition", s->position);
        }
    }

    scopeStack.pop();
    namespaceTable->pop();
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

    // Assign Expr
    else if(auto s = dynamic_cast<const AssignmentExpr*>(expr))
    { 
        return analyzeAssignmentExpr(s);
    }

    //OpComposedAssignmentExpr
    else if(auto s = dynamic_cast<const OpComposedAssignmentExpr*>(expr))
    {
        return analyzeOpComposedAssignmentExpr(s);
    }

    // Variable Expression
    else if(auto s = dynamic_cast<const VariableExpr*>(expr))
    {
        return analyzeVariableExpr(s);
    }

    //Array access Expression
    else if(auto s = dynamic_cast<const ArrayAccessExpr*>(expr))
    {
        return analyzeArrayAccessExpr(s);
    }

    //Array literal Expression
    else if(auto s = dynamic_cast<const LiteralArrayExpr*>(expr))
    {
        return analyzeLiteralArrayExpr(s);
    }

    // Function Call Expression
    else if(auto s = dynamic_cast<const CallExpr*>(expr))
    {   
        return analyzeCallExpr(s);
    }

    // Binary Expression
    else if(auto s = dynamic_cast<const BinaryExpr*>(expr))
    {
        return analyzeBinaryOperation(s);
    }

    // Unary Expression
    else if(auto s = dynamic_cast<const UnaryExpr*>(expr))
    {
        return analyzeUnaryExpr(s);
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

ExprAnalysisResult SemanticAnalyzer::analyzeAssignmentExpr(const AssignmentExpr* s)
{
    ExprAnalysisResult targetResult = analyzeExpr(s->target.get());
    ExprAnalysisResult valueResult = analyzeExpr(s->value.get());

    if(!types::isAssignmentCompatible(targetResult.type, valueResult.type)) {
        errorLog->addError("tipo incompatibile nell'assegnazione a " + s->target_name + "  " +
                        "[confronto tra " + types::toString(targetResult.type) + " e " 
                        + types::toString(valueResult.type) + "]", s->position);
        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }

    if(auto varExpr = dynamic_cast<const VariableExpr*>(s->target.get()))
    {
        if(targetResult.isConst) {
            errorLog->addError("forbidden assignment of const variable '" + varExpr->name + "'", s->position);
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        }
    }

    return targetResult;
}

ExprAnalysisResult SemanticAnalyzer::analyzeOpComposedAssignmentExpr(const OpComposedAssignmentExpr* s)
{
    ExprAnalysisResult targetResult = analyzeExpr(s->assignment->target.get());
    ExprAnalysisResult valueResult = analyzeExpr(s->assignment->value.get());

    PrimitiveType resultType = types::binaryResultType(s->op, targetResult.type.asPrimitive(), valueResult.type.asPrimitive());

    if(resultType == PrimitiveType::Error) {
        errorLog->addError("invalid operator " + typeToString(s->op) + "= for specified operands. " +
                           "operands types are " + types::toString(targetResult.type) + " and " + types::toString(valueResult.type), s->position);
        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }   

    ExprAnalysisResult assignResult = analyzeAssignmentExpr(s->assignment.get());

    if(assignResult.type.isError()) 
        return ExprAnalysisResult(Type(PrimitiveType::Error));

    return assignResult;
}

ExprAnalysisResult SemanticAnalyzer::analyzeVariableExpr(const VariableExpr* s)
{
    auto symbol = lookupSymbol(s->qualifiers, s->name);

    if(!symbol) {
        errorLog->addError("undefined symbol: " + s->name, s->position);
        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }

    return std::visit(SymbolVisitor{
        [] (const VariableSymbol& v) 
        {
            return ExprAnalysisResult(v.type, v.isConst);
        },
        [&] (const FunctionSymbol& f) {
            errorLog->addError("'" + s->name + "' is a function, cannot be used as a variable", s->position);
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        }
    }, symbol->category);
}

ExprAnalysisResult SemanticAnalyzer::analyzeArrayAccessExpr(const ArrayAccessExpr* s)
{
    auto baseResult = analyzeExpr(s->base.get());
    auto indexResult = analyzeExpr(s->index.get());

    if(baseResult.type.isError()) {
        return ExprAnalysisResult{Type(PrimitiveType::Error)};
    }

    if(!baseResult.type.isArray()) {
        errorLog->addError("indexing a non-array type (" + types::toString(baseResult.type) + ")", s->position);
        return ExprAnalysisResult{Type(PrimitiveType::Error)};
    }

    if(!indexResult.type.is(PrimitiveType::Int)) {
        errorLog->addError("array index must be of type integer", s->position);
    }

    PrimitiveType elementType = std::get<ArrayType>(baseResult.type.category).elementType;

    return ExprAnalysisResult(Type(elementType), baseResult.isConst);
}

ExprAnalysisResult SemanticAnalyzer::analyzeLiteralArrayExpr(const LiteralArrayExpr* s)
{
    if(s->elements.empty()) {
        errorLog->addError("could not convert empty enclosed-bracket to an array", s->position);
        return ExprAnalysisResult{Type(PrimitiveType::Error)};
    }

    auto arrayType = analyzeExpr(s->elements.at(0).get()).type;

    for(int i=1; i < s->elements.size(); ++i)
    {
        auto type = analyzeExpr(s->elements.at(i).get()).type;

        if(type.asPrimitive() != arrayType.asPrimitive()) {
            errorLog->addError("incompatible element of type " + types::toString(type) +
                                " in literal array of type " + types::toString(arrayType) + 
                                " at index " + std::to_string(i), s->position);
            return ExprAnalysisResult{Type(PrimitiveType::Error)};
        }
    }

    auto elementType = arrayType.asPrimitive();
    ArrayType arr(elementType, s->elements.size());

    //salva il valore ricostruito nell'espressione
    s->type = arr; 

    return ExprAnalysisResult{Type{arr}};
}

ExprAnalysisResult SemanticAnalyzer::analyzeCallExpr(const CallExpr* s)
{
    auto symbol = lookupSymbol(s->qualifiers, s->name);

    if(!symbol) {
        errorLog->addError("undefined symbol: " + s->name, s->position);
        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }

    return std::visit(SymbolVisitor{
        [&](const FunctionSymbol& f) 
        {
            if(s->args.size() != f.paramTypes.size()) {
                errorLog->addError("called function '" + s->name + "()" + "' required a different number of parameters than provided", s->position);
                return ExprAnalysisResult(Type(PrimitiveType::Error));
            }

            for(int i=0; i < s->args.size(); ++i) {
                auto res = analyzeExpr(s->args.at(i).get());

                if(!types::isAssignmentCompatible(f.paramTypes.at(i), res.type)) {
                    errorLog->addError("incopatible parameter type in function call to " + s->name, s->position);
                    return ExprAnalysisResult(Type(PrimitiveType::Error));
                }
            }

            return ExprAnalysisResult{f.returnType};
        },
        [&](const VariableSymbol& v) {
            errorLog->addError("'" + s->name + "' is not a function, cannot be called", s->position);
            return ExprAnalysisResult(Type(PrimitiveType::Error));
        },
    }, symbol->category);
}

ExprAnalysisResult SemanticAnalyzer::analyzeUnaryExpr(const UnaryExpr* s)
{
    //recursive call
    ExprAnalysisResult operandResult = analyzeExpr(s->operand.get());

    Type resultType = Type(types::unaryResultType(s->op, operandResult.type.asPrimitive()));

    if (resultType.is(PrimitiveType::Error) && !operandResult.type.is(PrimitiveType::Error)) {
        errorLog->addError("invalid unary operator for type  " +
                           types::toString(operandResult.type), s->position);
    }

    return ExprAnalysisResult{resultType};
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
                types::toString(leftType) + " e " + types::toString(rightType), expr->position);

        return ExprAnalysisResult(Type(PrimitiveType::Error));
    }

    return ExprAnalysisResult(Type(resultType));
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

/*
 * Funzione di lookup per i simboli nelle possibili tabelle.
 * Questa funzione ritorna il simbolo relativo all'espressione analizzata, controllando
 * nella tabella corretta (distinguendo tra nomi qualificati per la tabella namespace o
 * nomi singoli per lo scopestack).
 */

std::optional<Symbol> SemanticAnalyzer::lookupSymbol(Qualifiers q, const std::string& name)
{
    if(q.empty()) {
        return scopeStack.lookupSymbol(name);
    }
    return namespaceTable->searchQualifiedName(q, name);
}



