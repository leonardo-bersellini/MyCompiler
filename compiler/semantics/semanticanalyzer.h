#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include <unordered_map>
#include <string>

#include "symbols.h"
#include "AbstractSintaxTree.h"
#include "errors/errorlog.h"

#include "utils/stack/scope_stack.h"
#include "utils/namespace/namespace.h"

// struttura di ritorno dell'analisi delle espressioni, racchiude i dati di analisi
struct ExprAnalysisResult {
    ExprAnalysisResult() = default;
    ExprAnalysisResult(const Type& t, const bool c = false) : type(t), isConst(c) {}
    Type type;
    bool isConst;
};

class SemanticAnalyzer
{
public:
    SemanticAnalyzer();

    void analyzeProgram(const Program& program, ErrorLog& errorLog);
    void assignNamespaceTable(NamespaceTable& namespaceTable);

private:
    // Permette una lista di scope diversi, insieme di tabelle dei simboli
    scope_stack<std::string, Symbol> scopeStack; 
        
    // Funzione corrente (se esiste)
    const FunctionSymbol* currentFunction = nullptr; 

    ErrorLog* errorLog;
    NamespaceTable* namespaceTable;

    int loopDepth = 0; //contatore dell'anidamento dei cicli iterativi

    void analyzeStmt(const Stmt* stmt);

    void analyzeBlockStmt(const BlockStmt* block);
    void analyzeAssignment(const AssignmentStmt* s);
    void analyzeDeclaration(const DeclarationStmt* s);
    void analyseFunction(const FunctionStmt* s);
    void analyzeReturn(const ReturnStmt* s);
    void analyzeIf(const IfStmt* s);
    void analyzeFor(const ForStmt* s);
    void analyzeWhile(const WhileStmt* s);
    void analyzeSwitch(const SwitchStmt* s);
    void analyzeNamespace(const NamespaceStmt* s);

    void analyzeCase(const CaseStmt* s, const PrimitiveType& switch_type);
    void analyzeDefault(const DefaultStmt* s);

    ExprAnalysisResult analyzeExpr(const Expr* expr);

    ExprAnalysisResult analyzeBinaryOperation(const BinaryExpr* expr);

    bool allPathsReturn(const Stmt* stmt) const;
};

#endif // SEMANTICANALYZER_H
