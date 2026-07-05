#ifndef PARSER_H
#define PARSER_H

#include <QList>
#include <memory>

#include "token.h"
#include "AbstractSintaxTree.h"
#include "errorlog.h"

#include "symbols.h"

class Parser
{
public:
    Parser();

    std::unique_ptr<Program> parseProgram(const QList<Token> &tokens, ErrorLog& errorLog);

private:
    QList<Token> tokens;
    int currentPos; //indice del token corrente
    ErrorLog* errorLog;

    Token peek(int offset = 0) const;
    Token advance();

    bool isAtEnd() const;
    bool isAtEnd(int pos) const;
    bool check(TokenType type) const;
    bool expect(TokenType type);

    std::unique_ptr<Stmt> parseStatement();

    std::unique_ptr<Stmt> parseBranchBody();

    std::unique_ptr<Stmt> parseScopeStmt();
    std::unique_ptr<Stmt> parseAssignStmt();
    std::unique_ptr<Stmt> parseDeclarationStmt();
    std::unique_ptr<Stmt> parseFunctionStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseWhileStmt();

    std::unique_ptr<Expr> parseExpr();

    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseComparison();

    std::unique_ptr<Expr> parseMathExpression();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
};

#endif // PARSER_H
