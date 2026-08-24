#ifndef ABSTRACTSINTAXTREE_H
#define ABSTRACTSINTAXTREE_H

#include <memory>
#include <vector>
#include <string>
#include <vector>
#include <iostream>

#include "token.h"
#include "symbols.h"

#include "utils/ansi.h"


// Expressions - produce un valore

class Expr {
public:
    virtual ~Expr() = default;
};

class NumberExpr : public Expr {
public:
    double value;
    bool isInteger; //distinzione double-int
};

class StringExpr : public Expr {
public:
    std::string value;
};

class CharExpr : public Expr {
public:
    char value;
};

class BooleanExpr : public Expr {
public:
    bool value;
};

class VariableExpr : public Expr {
public:
    std::string name;
};

class BinaryExpr : public Expr {
public:
    TokenType op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

class UnaryExpr : public Expr {
public:
    TokenType op;
    std::unique_ptr<Expr> operand;
};

class CallExpr : public Expr { //chiamata ad una funzione
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

class ErrorExpr : public Expr {
public:
    //void, expression placeholder
};

// Statements - esecuzione di azioni

class Stmt {
public:
    virtual ~Stmt() = default;
};

class AssignmentStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
};

class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
};

class DeclarationStmt : public Stmt {
public:
    ValueType type;
    std::string name;
    std::unique_ptr<Expr> initializer; //contiene le informazioni di un'eventuale inizializzazione
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition; //boolean condition
    std::unique_ptr<Stmt> thenBranch; //contenuto tra {}
    std::unique_ptr<Stmt> elseBranch; //può contenere un altro if
};

class ForStmt : public Stmt {
public:
    std::unique_ptr<Stmt> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> update;
    std::unique_ptr<Stmt> body;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

class BreakStmt : public Stmt {
};

class ContinueStmt : public Stmt {
};

class ErrorStmt : public Stmt {
public:
    //void, placeholder per error stmt
};

// Functions

struct FunctionParam {
    ValueType type;
    std::string name;
};

class FunctionStmt : public Stmt {
public:
    std::string name;
    ValueType returnType;
    std::vector<FunctionParam> params;
    std::unique_ptr<Stmt> body; // BlockStmt
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; //nullptr per 'return;'
};

// Program

class Program {
public:
    Program() {}
    std::vector<std::unique_ptr<Stmt>> statements;
};



// Funzioni di Debug

inline void printAST(const Expr* node, int depth = 0) {
    std::string indent(depth * 2, ' ');

    if (auto n = dynamic_cast<const NumberExpr*>(node)) {
        std::cout << indent << "NumberExpr:" << n->value << std::endl;
    }
    else if (auto n = dynamic_cast<const StringExpr*>(node)) {
        std::cout << indent << "StringExpr:" << "\"" + n->value + "\"" << std::endl;
    }
    else if (auto n = dynamic_cast<const CharExpr*>(node)) {
        std::cout << indent << "CharExpr:" << n->value << std::endl;
    }
    else if (auto n = dynamic_cast<const BooleanExpr*>(node)) {
        std::cout << indent << "BooleanExpr:" << (n->value ? "true" : "false") << std::endl;
    }
    else if (auto n = dynamic_cast<const VariableExpr*>(node)) {
        std::cout << indent << "VariableExpr:" << n->name << std::endl;
    }
    else if (auto n = dynamic_cast<const UnaryExpr*>(node)) {
        std::cout << indent << "UnaryExpr:" << typeToString(n->op) << std::endl;
        printAST(n->operand.get(), depth + 1);
    }
    else if (auto n = dynamic_cast<const BinaryExpr*>(node)) {
        std::cout << indent << "BinaryExpr:" << typeToString(n->op) << std::endl;
        printAST(n->left.get(), depth + 1);
        printAST(n->right.get(), depth + 1);
    }
    else if (auto n = dynamic_cast<const CallExpr*>(node)) {
        std::cout << indent << "CallExpr:" << n->name << std::endl;

        for (const auto& arg : n->args)
            printAST(arg.get(), depth + 1);
    }
    else if (dynamic_cast<const ErrorExpr*>(node)) {
        std::cout << indent << "ErrorExpr" << std::endl;
    }
    else {
        std::cout << indent << "Unknown Expr" << std::endl;
    }
}

inline void printStmt(const Stmt* stmt, int depth = 0) 
{
    std::cout << ansi::color::bright_black;

    if (!stmt) {
        std::cout << std::string(depth * 2, ' ') + "<null stmt>" << std::endl;
        return;
    }

    std::string indent(depth * 2, ' ');

    if (auto s = dynamic_cast<const AssignmentStmt*>(stmt)) {

        std::cout << indent << "AssignmentStmt:" << s->name << std::endl;
        printAST(s->value.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const ExpressionStmt*>(stmt)) {

        std::cout << indent << "ExpressionStmt" << std::endl;
        printAST(s->expr.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const DeclarationStmt*>(stmt)) {

        std::cout << indent << "DeclarationStmt:" << s->name << std::endl;

        if (s->initializer)
            printAST(s->initializer.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const BlockStmt*>(stmt)) {

        std::cout << indent << "BlockStmt" << std::endl;

        for (const auto& st : s->statements)
            printStmt(st.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const IfStmt*>(stmt)) {

        std::cout << indent << "IfStmt";

        std::cout << std::string((depth + 1) * 2, ' ') << "Condition:" << std::endl;
        printAST(s->condition.get(), depth + 2);

        std::cout << std::string((depth + 1) * 2, ' ') << "Then:" << std::endl;
        printStmt(s->thenBranch.get(), depth + 2);

        if (s->elseBranch) {
            std::cout << std::string((depth + 1) * 2, ' ') << "Else:" << std::endl;
            printStmt(s->elseBranch.get(), depth + 2);
        }
    }

    else if (auto s = dynamic_cast<const WhileStmt*>(stmt)) {

        std::cout << indent << "WhileStmt" << std::endl;

        std::cout << std::string((depth + 1) * 2, ' ') << "Condition:" << std::endl;
        printAST(s->condition.get(), depth + 2);

        std::cout << std::string((depth + 1) * 2, ' ') << "Body:" << std::endl;
        printStmt(s->body.get(), depth + 2);
    }

    else if (auto s = dynamic_cast<const ForStmt*>(stmt)) {

        std::cout << indent << "ForStmt" << std::endl;

        if (s->init) {
            std::cout << std::string((depth + 1) * 2, ' ') << "Init:" << std::endl;
            printStmt(s->init.get(), depth + 2);
        }

        if (s->condition) {
            std::cout << std::string((depth + 1) * 2, ' ') << "Condition:" << std::endl;
            printAST(s->condition.get(), depth + 2);
        }

        if (s->update) {
            std::cout << std::string((depth + 1) * 2, ' ') << "Update:" << std::endl;
            printAST(s->update.get(), depth + 2);
        }

        if (s->body) {
            std::cout << std::string((depth + 1) * 2, ' ') << "Body:" << std::endl;
            printStmt(s->body.get(), depth + 2);
        }
    }

    else if (auto s = dynamic_cast<const FunctionStmt*>(stmt)) {

        std::cout << indent << "FunctionStmt:" << s->name << std::endl;

        for (const auto& p : s->params)
            std::cout << std::string((depth + 1) * 2, ' ') << "Param:" << p.name << std::endl;

        printStmt(s->body.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const ReturnStmt*>(stmt)) {

        std::cout << indent << "ReturnStmt" << std::endl;

        if (s->value)
            printAST(s->value.get(), depth + 1);
    }

    else if (dynamic_cast<const BreakStmt*>(stmt)) {

        std::cout << indent << "BreakStmt" << std::endl;
    }

    else if (dynamic_cast<const ContinueStmt*>(stmt)) {

        std::cout << indent << "ContinueStmt" << std::endl;
    }

    else if (dynamic_cast<const ErrorStmt*>(stmt)) {

        std::cout << indent << "ErrorStmt" << std::endl;
    }

    else {

        std::cout << indent << "Unknown Stmt" << std::endl;
    }

    std::cout << ansi::color::reset;
}

#endif // ABSTRACTSINTAXTREE_H
