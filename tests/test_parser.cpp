#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

TEST_CASE("Parser costruisce una DeclarationStmt", "[parser]")
{
    ErrorLog errorLog;

    SECTION("dichiarazione con inizializzazione")
    {
        auto program = parseSource("int x = 5;", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
        REQUIRE(program->statements.size() == 1);

        auto decl = dynamic_cast<DeclarationStmt*>(program->statements[0].get());
        REQUIRE(decl != nullptr);
        REQUIRE(decl->name == "x");
        REQUIRE(decl->type.is(PrimitiveType::Int));
        REQUIRE(decl->initializer != nullptr);
        REQUIRE_FALSE(decl->isConst);
    }

    SECTION("dichiarazione senza inizializzazione")
    {
        auto program = parseSource("double y;", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
        auto decl = dynamic_cast<DeclarationStmt*>(program->statements[0].get());
        REQUIRE(decl != nullptr);
        REQUIRE(decl->type.is(PrimitiveType::Double));
        REQUIRE(decl->initializer == nullptr);
    }

    SECTION("dichiarazione const")
    {
        auto program = parseSource("const int z = 1;", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
        auto decl = dynamic_cast<DeclarationStmt*>(program->statements[0].get());
        REQUIRE(decl != nullptr);
        REQUIRE(decl->isConst);
    }
}

TEST_CASE("Parser distingue dichiarazione e funzione partendo dallo stesso TypeKeyword", "[parser]")
{
    ErrorLog errorLog;

    auto program = parseSource("int add(int a, int b) { return a; }", errorLog);

    REQUIRE_FALSE(errorLog.hasErrors());
    REQUIRE(program->statements.size() == 1);

    auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
    REQUIRE(fn != nullptr);
    REQUIRE(fn->name == "add");
    REQUIRE(fn->returnType.is(PrimitiveType::Int));
    REQUIRE(fn->params.size() == 2);
    REQUIRE(fn->params[0].name == "a");
    REQUIRE(fn->params[1].name == "b");
    REQUIRE(dynamic_cast<BlockStmt*>(fn->body.get()) != nullptr);
}

TEST_CASE("Parser costruisce correttamente un AssignmentStmt", "[parser]")
{
    ErrorLog errorLog;

    auto program = parseSource("int main() { x = 3; }", errorLog);
    auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
    auto body = dynamic_cast<BlockStmt*>(fn->body.get());

    auto assign = dynamic_cast<AssignmentStmt*>(body->statements[0].get());
    REQUIRE(assign != nullptr);
    REQUIRE(assign->target_name == "x");
    REQUIRE(dynamic_cast<NumberExpr*>(assign->value.get()) != nullptr);
}

TEST_CASE("Parser gestisce if/elif/else", "[parser]")
{
    ErrorLog errorLog;

    auto program = parseSource(
        "int main() { if (true) { return 1; } elif (false) { return 2; } else { return 3; } }",
        errorLog);

    REQUIRE_FALSE(errorLog.hasErrors());

    auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
    auto body = dynamic_cast<BlockStmt*>(fn->body.get());
    auto ifStmt = dynamic_cast<IfStmt*>(body->statements[0].get());

    REQUIRE(ifStmt != nullptr);
    REQUIRE(dynamic_cast<BooleanExpr*>(ifStmt->condition.get()) != nullptr);
    REQUIRE(ifStmt->thenBranch != nullptr);
    REQUIRE(ifStmt->elseBranch != nullptr);

    // l'elif si traduce in un IfStmt annidato dentro elseBranch
    auto elifStmt = dynamic_cast<IfStmt*>(ifStmt->elseBranch.get());
    REQUIRE(elifStmt != nullptr);
    REQUIRE(elifStmt->elseBranch != nullptr);
}

TEST_CASE("Parser gestisce while e for", "[parser]")
{
    ErrorLog errorLog;

    SECTION("while")
    {
        auto program = parseSource("int main() { while (true) { } }", errorLog);
        auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
        auto body = dynamic_cast<BlockStmt*>(fn->body.get());

        REQUIRE(dynamic_cast<WhileStmt*>(body->statements[0].get()) != nullptr);
    }

    SECTION("for")
    {
        auto program = parseSource("int main() { for (int i = 0; i < 10;) { } }", errorLog);
        auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
        auto body = dynamic_cast<BlockStmt*>(fn->body.get());

        auto forStmt = dynamic_cast<ForStmt*>(body->statements[0].get());
        REQUIRE(forStmt != nullptr);
        REQUIRE(forStmt->init != nullptr);
    }
}

TEST_CASE("Parser rispetta la precedenza degli operatori aritmetici e logici", "[parser]")
{
    ErrorLog errorLog;

    SECTION("* lega più stretto di +")
    {
        auto program = parseSource("int main() { 1 + 2 * 3; }", errorLog);
        auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
        auto body = dynamic_cast<BlockStmt*>(fn->body.get());
        auto exprStmt = dynamic_cast<ExpressionStmt*>(body->statements[0].get());

        auto top = dynamic_cast<BinaryExpr*>(exprStmt->expr.get());
        REQUIRE(top != nullptr);
        REQUIRE(top->op == TokenType::Plus);

        // il ramo destro deve essere '2 * 3', non '(1 + 2) * 3'
        auto right = dynamic_cast<BinaryExpr*>(top->right.get());
        REQUIRE(right != nullptr);
        REQUIRE(right->op == TokenType::Star);
    }

    SECTION("&& lega più stretto di ||")
    {
        auto program = parseSource("int main() { true || false && true; }", errorLog);
        auto fn = dynamic_cast<FunctionStmt*>(program->statements[0].get());
        auto body = dynamic_cast<BlockStmt*>(fn->body.get());
        auto exprStmt = dynamic_cast<ExpressionStmt*>(body->statements[0].get());

        auto top = dynamic_cast<BinaryExpr*>(exprStmt->expr.get());
        REQUIRE(top != nullptr);
        REQUIRE(top->op == TokenType::LogicalOr);

        auto right = dynamic_cast<BinaryExpr*>(top->right.get());
        REQUIRE(right != nullptr);
        REQUIRE(right->op == TokenType::LogicalAnd);
    }
}

TEST_CASE("Parser segnala errori di sintassi senza andare in crash", "[parser]")
{
    ErrorLog errorLog;

    SECTION("dichiarazione senza identifier valido")
    {
        auto program = parseSource("int ;", errorLog);
        REQUIRE(errorLog.hasErrors());
    }

    SECTION("return fuori da una funzione è comunque parsato come stmt")
    {
        ErrorLog localLog;
        auto program = parseSource("return 1;", localLog);
        REQUIRE(program->statements.size() == 1);
        REQUIRE(dynamic_cast<ReturnStmt*>(program->statements[0].get()) != nullptr);
    }
}