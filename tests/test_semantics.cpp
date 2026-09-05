#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

// NOTA: analyzeProgram richiede che il programma contenga una funzione 'main' (o uno dei suoi
// alias) che ritorni int, come unico tipo di statement ammesso a livello globale. Per questo
// ogni sorgente di test valido è incapsulato in "int main() { ... }".

TEST_CASE("Un programma minimo valido non produce errori", "[semantics]")
{
    ErrorLog errorLog;
    analyzeSource("int main() { return 0; }", errorLog);

    REQUIRE_FALSE(errorLog.hasErrors());
}

TEST_CASE("Regole di top-level", "[semantics]")
{
    SECTION("un programma senza main produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int foo() { return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("una dichiarazione a livello globale non è ammessa")
    {
        ErrorLog errorLog;
        analyzeSource("int x = 1; int main() { return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }
}

TEST_CASE("Dichiarazione di variabili", "[semantics]")
{
    SECTION("inizializzazione con tipo compatibile non produce errori")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = 5; return 0; }", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }

    SECTION("promozione int -> double nell'init è ammessa")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { double x = 5; return 0; }", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }

    SECTION("inizializzazione con tipo incompatibile produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = true; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("dichiarazione void produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { void x; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("ridichiarazione nello stesso scope produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = 1; int x = 2; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("const senza inizializzazione produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { const int x; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("stesso nome in scope annidati diversi è ammesso (shadowing)")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = 1; { int x = 2; } return 0; }", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }
}

TEST_CASE("Assegnazione di variabili", "[semantics]")
{
    SECTION("assegnazione a variabile non dichiarata produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { x = 1; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("assegnazione con tipo incompatibile produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = 1; x = true; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("assegnazione ad una variabile const produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { const int x = 1; x = 2; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }
}

TEST_CASE("Controllo dei percorsi di ritorno di una funzione", "[semantics]")
{
    SECTION("funzione non-void con return solo in un ramo dell'if produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource(
            "int helper(bool cond) { if (cond) { return 1; } } "
            "int main() { return 0; }",
            errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("funzione non-void con return su entrambi i rami dell'if è valida")
    {
        ErrorLog errorLog;
        analyzeSource(
            "int helper(bool cond) { if (cond) { return 1; } else { return 2; } } "
            "int main() { return 0; }",
            errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }

    SECTION("funzione void non richiede return")
    {
        ErrorLog errorLog;
        analyzeSource("void helper() { } int main() { return 0; }", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }
}

TEST_CASE("Chiamate di funzione", "[semantics]")
{
    SECTION("numero di argomenti errato produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource(
            "int add(int a, int b) { return a; } "
            "int main() { add(1); return 0; }",
            errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("tipo di argomento incompatibile produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource(
            "int add(int a, int b) { return a; } "
            "int main() { add(1, true); return 0; }",
            errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("chiamata a funzione non dichiarata produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { notDeclared(); return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("chiamata valida non produce errori")
    {
        ErrorLog errorLog;
        analyzeSource(
            "int add(int a, int b) { return a + b; } "
            "int main() { int r = add(1, 2); return 0; }",
            errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }
}

TEST_CASE("Operatori binari e unari", "[semantics]")
{
    SECTION("somma tra int e bool produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { int x = 1 + true; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }

    SECTION("confronto relazionale tra numerici è valido")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { bool r = 1 < 2; return 0; }", errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
    }

    SECTION("operatore logico ! su tipo non booleano produce un errore")
    {
        ErrorLog errorLog;
        analyzeSource("int main() { bool r = !1; return 0; }", errorLog);

        REQUIRE(errorLog.hasErrors());
    }
}
