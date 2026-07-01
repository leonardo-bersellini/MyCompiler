#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include <QMap>
#include <QString>

#include "symbol.h"
#include "AbstractSintaxTree.h"
#include "errorlog.h"

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
    QList<QMap<QString, SymbolInfo>> scopeStack; //permette una lista di scope diversi
    ErrorLog* errorLog;

    int loopDepth = 0; //contatore dell'anidamento dei cicli iterativi


    bool isAssignmentCompatible(ValueType variableType, ValueType assignType);

    void analyzeStmt(const Stmt* stmt);
    ExprAnalysisResult analyzeExpr(const Expr* expr);

    ExprAnalysisResult analyzeBinaryOperation(const BinaryExpr* expr);
    bool isNumeric(ValueType t);


    bool symbolExistsAnywhere(const QString& name) const;
    bool symbolExistsInCurrentScope(const QString& name) const;
    SymbolInfo lookupSymbolInfo(const QString& name) const;
    void declareSymbol(const QString& name, SymbolInfo info);
    void pushScope();
    void popScope();
};

#endif // SEMANTICANALYZER_H
