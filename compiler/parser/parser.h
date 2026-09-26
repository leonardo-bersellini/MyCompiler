#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <concepts>

#include "token.h"
#include "AbstractSintaxTree.h"
#include "errors/errorlog.h"
#include "errors/recovery/recoveryhandler.h"

#include "symbols.h"

class Parser
{
public:
    Parser();

    std::unique_ptr<Program> parseProgram(const std::vector<Token> &tokens, ErrorLog& errorLog);

private:
    std::vector<Token> tokens;
    int currentPos; //indice del token corrente

    ErrorLog* errorLog;
    RecoveryHandler recoveryHandler;

    Token peek(int offset = 0) const;
    Token advance();

    bool isAtEnd() const;
    bool isAtEnd(int pos) const;
    bool check(TokenType type) const;
    bool expect(TokenType type, bool applyGhostRecovery = false);

    template<typename T>
        requires std::derived_from<T, Stmt>
    std::unique_ptr<T> make_unique_stmt();

    template<typename T>
        requires std::derived_from<T, Expr>
    std::unique_ptr<T> make_unique_expr();

    std::unique_ptr<Stmt> parseStatement();

    std::unique_ptr<Stmt> parseBranchBody();
    Type parseArrayType(); 
    std::pair<std::string, Qualifiers> resolveQualifiedName();

    std::unique_ptr<Stmt> parseScopeStmt();
    std::unique_ptr<Stmt> parseDeclarationStmt(bool isConstDeclaration = false);
    std::unique_ptr<Stmt> parseFunctionStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseSwitchStmt();
    std::unique_ptr<CaseStmt> parseCaseStmt();
    std::unique_ptr<DefaultStmt> parseDefaultStmt();
    std::unique_ptr<Stmt> parseNamespaceStmt();
    std::unique_ptr<Stmt> parsePrintStmt();
    

    std::unique_ptr<Expr> parseExpr();

    std::unique_ptr<Expr> parseAssignment();

    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseComparison();

    std::unique_ptr<Expr> parseMathExpression();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
};

#endif // PARSER_H
