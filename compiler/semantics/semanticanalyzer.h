#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include <unordered_map>
#include <string>

#include "symbols.h"
#include "AbstractSintaxTree.h"
#include "errors/errorlog.h"

// struttura di ritorno dell'analisi delle espressioni, racchiude i dati di analisi
struct ExprAnalysisResult {
    ValueType value_type;
    // futuro: bool isConstant; std::optional<double> constantValue; ecc.
};

class SemanticAnalyzer
{
public:
    SemanticAnalyzer();

    void analyzeProgram(const Program& program, ErrorLog& errorLog);

private:
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopeStack; // Permette una lista di scope diversi, insieme di tabelle dei simboli

    std::unordered_map<std::string, FunctionInfo> functionTable;     // Tabella delle funzioni dichiarate
    const FunctionInfo* currentFunction = nullptr; // Funzione corrente (se esiste)

    ErrorLog* errorLog;

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

    void analyzeCase(const CaseStmt* s, const ValueType& switch_type);
    void analyzeDefault(const DefaultStmt* s);

    ExprAnalysisResult analyzeExpr(const Expr* expr);

    ExprAnalysisResult analyzeBinaryOperation(const BinaryExpr* expr);

    bool symbolExistsAnywhere(const std::string& name) const;
    bool symbolExistsInCurrentScope(const std::string& name) const;
    SymbolInfo lookupSymbolInfo(const std::string& name) const;
    void declareSymbol(const std::string& name, SymbolInfo info);
    void pushScope();
    void popScope();

    bool allPathsReturn(const Stmt* stmt) const;
};

#endif // SEMANTICANALYZER_H
