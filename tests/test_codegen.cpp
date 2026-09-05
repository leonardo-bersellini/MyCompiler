#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"
#include "codegen/codegenerator.h"

// NOTA IMPORTANTE:
// CodeGenerator non espone alcun accessor pubblico al llvm::Module generato (è privato),
// quindi da qui non è possibile ispezionare l'IR prodotto per fare assert puntuali
// (es. "la funzione main contiene una add"). Questi test sono quindi smoke test:
// verificano che generate() non vada in crash/assert su programmi semanticamente validi.
// Se in futuro vuoi test più precisi sull'IR generato, serve un getter (anche solo per i test,
// es. tramite una build di CodeGenerator con friend class o un metodo dumpIRToString()).

namespace
{
    // esegue l'intera pipeline lexer -> parser -> semantics e ritorna il Program,
    // fallendo il test se qualche fase precedente al codegen produce errori.
    std::unique_ptr<Program> buildValidProgram(const std::string& source)
    {
        ErrorLog errorLog;
        auto program = analyzeSource(source, errorLog);
        REQUIRE_FALSE(errorLog.hasErrors());
        return program;
    }
}

TEST_CASE("CodeGenerator genera senza crash un programma minimo", "[codegen]")
{
    auto program = buildValidProgram("int main() { return 0; }");

    CodeGenerator codegen;
    REQUIRE_NOTHROW(codegen.generate(*program));
}

TEST_CASE("CodeGenerator genera senza crash dichiarazioni ed espressioni aritmetiche", "[codegen]")
{
    auto program = buildValidProgram(
        "int main() { int x = 5; double y = x + 2.5; return 0; }");

    CodeGenerator codegen;
    REQUIRE_NOTHROW(codegen.generate(*program));
}

TEST_CASE("CodeGenerator genera senza crash if/else e cicli", "[codegen]")
{
    auto program = buildValidProgram(
        "int main() { "
        "  int i = 0; "
        "  while (i < 10) { "
        "    if (i == 5) { i = i + 2; } else { i = i + 1; } "
        "  } "
        "  return 0; "
        "}");

    CodeGenerator codegen;
    REQUIRE_NOTHROW(codegen.generate(*program));
}

TEST_CASE("CodeGenerator genera senza crash una chiamata di funzione con parametri", "[codegen]")
{
    auto program = buildValidProgram(
        "int add(int a, int b) { return a + b; } "
        "int main() { int r = add(2, 3); return 0; }");

    CodeGenerator codegen;
    REQUIRE_NOTHROW(codegen.generate(*program));
}
