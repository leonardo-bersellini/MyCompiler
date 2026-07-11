#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include <QMap>
#include <QString>

#include "symbols.h"
#include "AbstractSintaxTree.h"
#include "errorlog/errorlog.h"

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
    QList<QMap<QString, SymbolInfo>> scopeStack; // Permette una lista di scope diversi, insieme di tabelle dei simboli

    QMap<QString, FunctionInfo> functionTable;     // Tabella delle funzioni dichiarate
    const FunctionInfo* currentFunction = nullptr; // Funzione corrente (se esiste)

    ErrorLog* errorLog;

    int loopDepth = 0; //contatore dell'anidamento dei cicli iterativi

    void analyzeStmt(const Stmt* stmt);
    ExprAnalysisResult analyzeExpr(const Expr* expr);

    ExprAnalysisResult analyzeBinaryOperation(const BinaryExpr* expr);

    bool symbolExistsAnywhere(const QString& name) const;
    bool symbolExistsInCurrentScope(const QString& name) const;
    SymbolInfo lookupSymbolInfo(const QString& name) const;
    void declareSymbol(const QString& name, SymbolInfo info);
    void pushScope();
    void popScope();
};

#endif // SEMANTICANALYZER_H
