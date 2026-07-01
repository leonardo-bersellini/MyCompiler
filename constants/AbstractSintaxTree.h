#ifndef ABSTRACTSINTAXTREE_H
#define ABSTRACTSINTAXTREE_H

#include "token.h"
#include "symbol.h"
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
        qDebug() << indent + "NumberExpr:" << n->value;
    }
    else if (auto n = dynamic_cast<const VariableExpr*>(node)) {
        qDebug() << indent + "VariableExpr:" << n->name;
    }
    else if (auto n = dynamic_cast<const UnaryExpr*>(node)) {
        qDebug() << indent + "UnaryExpr op=" << static_cast<QString>(typeToString(n->op));
        printAST(n->operand.get(), depth + 1);
    }
    else if (auto n = dynamic_cast<const BinaryExpr*>(node)) {
        qDebug() << indent + "BinaryExpr op=" << static_cast<QString>(typeToString(n->op));
        printAST(n->left.get(), depth + 1);
        printAST(n->right.get(), depth + 1);
    }
    else if (dynamic_cast<const ErrorExpr*>(node)) {
        qDebug() << indent + "ErrorExpr";
    }
    else {
        qDebug() << indent + "Unknown node";
    }
}

inline void printStmt(const Stmt* stmt, int depth = 0) {
    QString indent(depth * 2, ' ');

    if (auto s = dynamic_cast<const AssignmentStmt*>(stmt)) {
        qDebug() << indent + "AssignmentStmt:" << s->name;
        printAST(s->value.get(), depth + 1);
    }
    else if (auto s = dynamic_cast<const ExpressionStmt*>(stmt)) {
        qDebug() << indent + "ExpressionStmt";
        printAST(s->expr.get(), depth + 1);
    }
}

#endif // ABSTRACTSINTAXTREE_H
