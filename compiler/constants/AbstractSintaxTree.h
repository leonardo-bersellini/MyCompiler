#ifndef ABSTRACTSINTAXTREE_H
#define ABSTRACTSINTAXTREE_H

#include "token.h"
#include "symbols.h"
#include <memory>
#include <vector>
#include <QString>
#include <QList>
#include <QDebug>

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
    QString value;
};

class CharExpr : public Expr {
public:
    QChar value;
};

class BooleanExpr : public Expr {
public:
    bool value;
};

class VariableExpr : public Expr {
public:
    QString name;
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
    QString name;
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
    QString name;
    std::unique_ptr<Expr> value;
};

class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
};

class DeclarationStmt : public Stmt {
public:
    ValueType type;
    QString name;
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
    QString name;
};

class FunctionStmt : public Stmt {
public:
    QString name;
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
    QString indent(depth * 2, ' ');

    if (auto n = dynamic_cast<const NumberExpr*>(node)) {
        qDebug().noquote() << indent << "NumberExpr:" << n->value;
    }
    else if (auto n = dynamic_cast<const StringExpr*>(node)) {
        qDebug().noquote() << indent << "StringExpr:" << "\"" + n->value + "\"";
    }
    else if (auto n = dynamic_cast<const CharExpr*>(node)) {
        qDebug().noquote() << indent << "CharExpr:" << n->value;
    }
    else if (auto n = dynamic_cast<const BooleanExpr*>(node)) {
        qDebug().noquote() << indent << "BooleanExpr:" << (n->value ? "true" : "false");
    }
    else if (auto n = dynamic_cast<const VariableExpr*>(node)) {
        qDebug().noquote() << indent << "VariableExpr:" << n->name;
    }
    else if (auto n = dynamic_cast<const UnaryExpr*>(node)) {
        qDebug().noquote() << indent << "UnaryExpr:" << typeToString(n->op);
        printAST(n->operand.get(), depth + 1);
    }
    else if (auto n = dynamic_cast<const BinaryExpr*>(node)) {
        qDebug().noquote() << indent << "BinaryExpr:" << typeToString(n->op);
        printAST(n->left.get(), depth + 1);
        printAST(n->right.get(), depth + 1);
    }
    else if (auto n = dynamic_cast<const CallExpr*>(node)) {
        qDebug().noquote() << indent << "CallExpr:" << n->name;

        for (const auto& arg : n->args)
            printAST(arg.get(), depth + 1);
    }
    else if (dynamic_cast<const ErrorExpr*>(node)) {
        qDebug().noquote() << indent << "ErrorExpr";
    }
    else {
        qDebug().noquote() << indent << "Unknown Expr";
    }
}

inline void printStmt(const Stmt* stmt, int depth = 0) {
    if (!stmt) {
        qDebug().noquote() << QString(depth * 2, ' ') + "<null stmt>";
        return;
    }

    QString indent(depth * 2, ' ');

    if (auto s = dynamic_cast<const AssignmentStmt*>(stmt)) {

        qDebug().noquote() << indent << "AssignmentStmt:" << s->name;
        printAST(s->value.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const ExpressionStmt*>(stmt)) {

        qDebug().noquote() << indent << "ExpressionStmt";
        printAST(s->expr.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const DeclarationStmt*>(stmt)) {

        qDebug().noquote() << indent << "DeclarationStmt:"
                           << s->name;

        if (s->initializer)
            printAST(s->initializer.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const BlockStmt*>(stmt)) {

        qDebug().noquote() << indent << "BlockStmt";

        for (const auto& st : s->statements)
            printStmt(st.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const IfStmt*>(stmt)) {

        qDebug().noquote() << indent << "IfStmt";

        qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Condition:";
        printAST(s->condition.get(), depth + 2);

        qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Then:";
        printStmt(s->thenBranch.get(), depth + 2);

        if (s->elseBranch) {
            qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Else:";
            printStmt(s->elseBranch.get(), depth + 2);
        }
    }

    else if (auto s = dynamic_cast<const WhileStmt*>(stmt)) {

        qDebug().noquote() << indent << "WhileStmt";

        qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Condition:";
        printAST(s->condition.get(), depth + 2);

        qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Body:";
        printStmt(s->body.get(), depth + 2);
    }

    else if (auto s = dynamic_cast<const ForStmt*>(stmt)) {

        qDebug().noquote() << indent << "ForStmt";

        if (s->init) {
            qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Init:";
            printStmt(s->init.get(), depth + 2);
        }

        if (s->condition) {
            qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Condition:";
            printAST(s->condition.get(), depth + 2);
        }

        if (s->update) {
            qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Update:";
            printAST(s->update.get(), depth + 2);
        }

        if (s->body) {
            qDebug().noquote() << QString((depth + 1) * 2, ' ') << "Body:";
            printStmt(s->body.get(), depth + 2);
        }
    }

    else if (auto s = dynamic_cast<const FunctionStmt*>(stmt)) {

        qDebug().noquote() << indent << "FunctionStmt:" << s->name;

        for (const auto& p : s->params)
            qDebug().noquote() << QString((depth + 1) * 2, ' ')
                               << "Param:" << p.name;

        printStmt(s->body.get(), depth + 1);
    }

    else if (auto s = dynamic_cast<const ReturnStmt*>(stmt)) {

        qDebug().noquote() << indent << "ReturnStmt";

        if (s->value)
            printAST(s->value.get(), depth + 1);
    }

    else if (dynamic_cast<const BreakStmt*>(stmt)) {

        qDebug().noquote() << indent << "BreakStmt";
    }

    else if (dynamic_cast<const ContinueStmt*>(stmt)) {

        qDebug().noquote() << indent << "ContinueStmt";
    }

    else if (dynamic_cast<const ErrorStmt*>(stmt)) {

        qDebug().noquote() << indent << "ErrorStmt";
    }

    else {

        qDebug().noquote() << indent << "Unknown Stmt";
    }
}

#endif // ABSTRACTSINTAXTREE_H
