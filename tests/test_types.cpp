#include <catch2/catch_test_macros.hpp>

#include "types.h"
#include "token.h"

TEST_CASE("isNumeric e isTextual classificano correttamente i tipi primitivi", "[types]")
{
    SECTION("Int e Double sono numerici")
    {
        REQUIRE(types::isNumeric(PrimitiveType::Int));
        REQUIRE(types::isNumeric(PrimitiveType::Double));
    }

    SECTION("Char, Bool, String, Void non sono numerici")
    {
        REQUIRE_FALSE(types::isNumeric(PrimitiveType::Char));
        REQUIRE_FALSE(types::isNumeric(PrimitiveType::Bool));
        REQUIRE_FALSE(types::isNumeric(PrimitiveType::String));
        REQUIRE_FALSE(types::isNumeric(PrimitiveType::Void));
    }

    SECTION("Char e String sono testuali")
    {
        REQUIRE(types::isTextual(PrimitiveType::Char));
        REQUIRE(types::isTextual(PrimitiveType::String));
    }

    SECTION("Int e Bool non sono testuali")
    {
        REQUIRE_FALSE(types::isTextual(PrimitiveType::Int));
        REQUIRE_FALSE(types::isTextual(PrimitiveType::Bool));
    }
}

TEST_CASE("isAssignmentCompatible su tipi primitivi", "[types]")
{
    SECTION("stesso tipo è sempre compatibile")
    {
        REQUIRE(types::isAssignmentCompatible(Type{PrimitiveType::Int}, Type{PrimitiveType::Int}));
        REQUIRE(types::isAssignmentCompatible(Type{PrimitiveType::Bool}, Type{PrimitiveType::Bool}));
    }

    SECTION("promozione Int -> Double è ammessa")
    {
        REQUIRE(types::isAssignmentCompatible(Type{PrimitiveType::Double}, Type{PrimitiveType::Int}));
    }

    SECTION("Double -> Int NON è ammesso (nessun troncamento implicito)")
    {
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{PrimitiveType::Int}, Type{PrimitiveType::Double}));
    }

    SECTION("tipi incompatibili vengono rifiutati")
    {
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{PrimitiveType::Int}, Type{PrimitiveType::Bool}));
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{PrimitiveType::String}, Type{PrimitiveType::Int}));
    }

    SECTION("un Error da un lato è sempre compatibile (errore già segnalato altrove)")
    {
        REQUIRE(types::isAssignmentCompatible(Type{PrimitiveType::Error}, Type{PrimitiveType::Int}));
        REQUIRE(types::isAssignmentCompatible(Type{PrimitiveType::Int}, Type{PrimitiveType::Error}));
    }
}

TEST_CASE("isAssignmentCompatible su array", "[types]")
{
    ArrayType a3int(PrimitiveType::Int, 3);
    ArrayType a4int(PrimitiveType::Int, 4);
    ArrayType a3double(PrimitiveType::Double, 3);

    SECTION("stesso elementType e stessa size sono compatibili")
    {
        REQUIRE(types::isAssignmentCompatible(Type{a3int}, Type{a3int}));
    }

    SECTION("size diversa non è compatibile")
    {
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{a3int}, Type{a4int}));
    }

    SECTION("elementType diverso non è compatibile (niente promozione negli array)")
    {
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{a3int}, Type{a3double}));
    }

    SECTION("mix array/primitivo non è mai compatibile")
    {
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{a3int}, Type{PrimitiveType::Int}));
        REQUIRE_FALSE(types::isAssignmentCompatible(Type{PrimitiveType::Int}, Type{a3int}));
    }
}

TEST_CASE("additionResultType gestisce numerico e testuale", "[types]")
{
    SECTION("Int + Int = Int")
    {
        REQUIRE(types::additionResultType(PrimitiveType::Int, PrimitiveType::Int) == PrimitiveType::Int);
    }

    SECTION("Int + Double = Double")
    {
        REQUIRE(types::additionResultType(PrimitiveType::Int, PrimitiveType::Double) == PrimitiveType::Double);
    }

    SECTION("Char + Char = String")
    {
        REQUIRE(types::additionResultType(PrimitiveType::Char, PrimitiveType::Char) == PrimitiveType::String);
    }

    SECTION("String + Char = String")
    {
        REQUIRE(types::additionResultType(PrimitiveType::String, PrimitiveType::Char) == PrimitiveType::String);
    }

    SECTION("Bool + Bool = Error (non numerico, non testuale)")
    {
        REQUIRE(types::additionResultType(PrimitiveType::Bool, PrimitiveType::Bool) == PrimitiveType::Error);
    }
}

TEST_CASE("arithmeticResultType richiede operandi numerici", "[types]")
{
    SECTION("Int - Double = Double")
    {
        REQUIRE(types::arithmeticResultType(PrimitiveType::Int, PrimitiveType::Double) == PrimitiveType::Double);
    }

    SECTION("Int * Int = Int")
    {
        REQUIRE(types::arithmeticResultType(PrimitiveType::Int, PrimitiveType::Int) == PrimitiveType::Int);
    }

    SECTION("Bool / Int = Error")
    {
        REQUIRE(types::arithmeticResultType(PrimitiveType::Bool, PrimitiveType::Int) == PrimitiveType::Error);
    }
}

TEST_CASE("binaryResultType instrada l'operatore verso la regola corretta", "[types]")
{
    SECTION("== tra numerici -> Bool")
    {
        REQUIRE(types::binaryResultType(TokenType::EqualEqual, PrimitiveType::Int, PrimitiveType::Double) == PrimitiveType::Bool);
    }

    SECTION("< tra tipi non confrontabili -> Error")
    {
        REQUIRE(types::binaryResultType(TokenType::Less, PrimitiveType::Bool, PrimitiveType::String) == PrimitiveType::Error);
    }

    SECTION("&& tra Bool -> Bool")
    {
        REQUIRE(types::binaryResultType(TokenType::LogicalAnd, PrimitiveType::Bool, PrimitiveType::Bool) == PrimitiveType::Bool);
    }

    SECTION("&& con un operando non Bool -> Error")
    {
        REQUIRE(types::binaryResultType(TokenType::LogicalAnd, PrimitiveType::Bool, PrimitiveType::Int) == PrimitiveType::Error);
    }

    SECTION("+ tra numerici -> arithmeticResultType/additionResultType coerente")
    {
        REQUIRE(types::binaryResultType(TokenType::Plus, PrimitiveType::Int, PrimitiveType::Int) == PrimitiveType::Int);
    }

    SECTION("un operando Error si propaga sempre come Error")
    {
        REQUIRE(types::binaryResultType(TokenType::Plus, PrimitiveType::Error, PrimitiveType::Int) == PrimitiveType::Error);
    }
}

TEST_CASE("unaryResultType", "[types]")
{
    SECTION("! su Bool -> Bool")
    {
        REQUIRE(types::unaryResultType(TokenType::LogicalNot, PrimitiveType::Bool) == PrimitiveType::Bool);
    }

    SECTION("! su Int -> Error")
    {
        REQUIRE(types::unaryResultType(TokenType::LogicalNot, PrimitiveType::Int) == PrimitiveType::Error);
    }

    SECTION("- su Double -> Double (tipo invariato)")
    {
        REQUIRE(types::unaryResultType(TokenType::Minus, PrimitiveType::Double) == PrimitiveType::Double);
    }

    SECTION("- su String -> Error")
    {
        REQUIRE(types::unaryResultType(TokenType::Minus, PrimitiveType::String) == PrimitiveType::Error);
    }
}

TEST_CASE("promotionType", "[types]")
{
    SECTION("stesso tipo resta invariato")
    {
        REQUIRE(types::promotionType(PrimitiveType::Int, PrimitiveType::Int) == PrimitiveType::Int);
    }

    SECTION("Int e Double promuovono sempre a Double")
    {
        REQUIRE(types::promotionType(PrimitiveType::Int, PrimitiveType::Double) == PrimitiveType::Double);
        REQUIRE(types::promotionType(PrimitiveType::Double, PrimitiveType::Int) == PrimitiveType::Double);
    }

    SECTION("tipi non numerici e diversi -> Error")
    {
        REQUIRE(types::promotionType(PrimitiveType::Bool, PrimitiveType::Char) == PrimitiveType::Error);
    }
}

TEST_CASE("toString e toPrimitiveType sono coerenti tra loro", "[types]")
{
    SECTION("round-trip sui tipi primitivi base")
    {
        REQUIRE(types::toString(Type{PrimitiveType::Int}) == "int");
        REQUIRE(types::toPrimitiveType("int") == PrimitiveType::Int);

        REQUIRE(types::toString(Type{PrimitiveType::Double}) == "double");
        REQUIRE(types::toPrimitiveType("double") == PrimitiveType::Double);
    }

    SECTION("nome sconosciuto -> Error")
    {
        REQUIRE(types::toPrimitiveType("nonesiste") == PrimitiveType::Error);
    }

    SECTION("toString su ArrayType include elementType e size")
    {
        Type arr{ArrayType(PrimitiveType::Int, 5)};
        REQUIRE(types::toString(arr) == "int[5]");
    }
}
