#include <catch2/catch_test_macros.hpp>

#include "test_helpers.h"

#include "codegen/codegenerator.h"

TEST_CASE("Testing scripts for bismuth code", "[integration]")
{
    SECTION("Simple script")
    {
        const std::string input = "int main() { int a = 9; const int b = 10; return 0;}";

        ErrorLog errorLog;
        CodeGenerator codegen;
        NamespaceTable ns;
        codegen.assignNamespaceTable(ns);
        auto program = analyzeSource(input, errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
        REQUIRE_NOTHROW(codegen.generate(*program.get()));
    }

    SECTION("Medium script with arrays")
    {
        const std::string input = "int main() {"
                                    "int v_A = 9;"
                                    "int v_B = 10;"
                                    "int v_C = 281930;"
                                    "v_B = v_A;"
                                    "v_B = v_C;"
                                    "v_A = v_C;"
                                    "char[1] str = \"A\";"
                                    "double[1] strr = [3.14];"

                                    "int[2] arr;"
                                    "arr = [1, 2];"
                                    "int[2] arrw = [1, 9];"
                                    "arr = arrw;"
                                    "int[9] tmp;"
                                    "int[9] a = tmp;"
                                    "int[3] b = [1, 4, 6];"
                                    "int x = b[2];"

                                    "return 0; } //end main"

                                    "void F(int[5] a, bool c) { return;} ";

        ErrorLog errorLog;
        CodeGenerator codegen;
        NamespaceTable ns;
        codegen.assignNamespaceTable(ns);
        auto program = analyzeSource(input, errorLog);

        REQUIRE_FALSE(errorLog.hasErrors());
        REQUIRE_NOTHROW(codegen.generate(*program.get()));
    }
}