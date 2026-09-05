#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include "lexer/lexer.h"
#include "constants/keywords.h"

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

std::vector<Token> lexerTest(const std::string& input) {
    Lexer lexer;
    ErrorLog log;
    return lexer.analiseString(input, log);
}

TEST_CASE("Lexer _ controlli numerici", "[lexer]") 
{
    SECTION("intero letterale") 
    {
        auto [input, result] = GENERATE(
            std::make_pair(std::string("42"), double(42.0)),
            std::make_pair(std::string("56"), double(56.0)),
            std::make_pair(std::string("681249"), double(681249.0))
        );
        
        auto tokens = lexerTest(input);

        REQUIRE(tokens.at(0).type == TokenType::IntegerLiteral);
        REQUIRE(tokens.at(0).lexeme == input);
        REQUIRE(tokens.at(0).numericValue == result);
    }

    SECTION("double letterale")
    {
        auto [input, result] = GENERATE(
            std::make_pair(std::string("942.0"), double(942.0)),
            std::make_pair(std::string("134.0"), double(134.0)),
            std::make_pair(std::string("62409.0"), double(62409.0))
        );

        auto tokens = lexerTest(input);

        REQUIRE(tokens.at(0).type == TokenType::DoubleLiteral);
        REQUIRE(tokens.at(0).lexeme == input);
        REQUIRE(tokens.at(0).numericValue == result);
    }

}

TEST_CASE("Lexer _ controlli letterali", "[lexer]")
{
    SECTION("bool letterale")
    {
        auto input = GENERATE(
            std::string("true"),
            std::string("false")
        );

        auto tokens = lexerTest(input);

        REQUIRE(tokens.at(0).type == TokenType::BoolLiteral);
        REQUIRE(tokens.at(0).lexeme == input);
    }

    SECTION("char letterale")
    {
        auto input = GENERATE(
            std::string("'c'"),
            std::string("'a'"),
            std::string("'f'"),
            std::string("'8'")
        );

        auto tokens = lexerTest(input);

        REQUIRE(tokens.at(0).type == TokenType::CharLiteral);
        REQUIRE(tokens.at(0).lexeme == input.substr(1, input.size() - 2)); //rimuove primo e ultimo char
    }
}

TEST_CASE("Lexer _ keywords", "[lexer]")
{
    SECTION("keywords") 
    {
        auto [input, result] = GENERATE(
            from_range(keywords)
        );

        auto tokens = lexerTest(input);

        REQUIRE(tokens.at(0).lexeme == input);
        REQUIRE(tokens.at(0).type == result);
    }
}

TEST_CASE("Lexer _ complex expressions", "[lexer]")
{
    std::string input = "int A = (3 + 4) * 2";

    auto tokens = lexerTest(input);

    REQUIRE(tokens.at(0).type == TokenType::TypeKeyword);
    REQUIRE(tokens.at(1).type == TokenType::Identifier);
    REQUIRE(tokens.at(2).type == TokenType::Equal);
    REQUIRE(tokens.at(4).type == TokenType::IntegerLiteral);
    REQUIRE(tokens.at(5).type == TokenType::Plus);
    REQUIRE(tokens.at(9).type == TokenType::IntegerLiteral);
}