#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <unordered_map>
#include <stdexcept>

struct TextPosition {
    int line;
    int column;
    std::string source_file;
};

enum class TokenType {
    IntegerLiteral,
    DoubleLiteral,
    StringLiteral,
    CharLiteral,
    BoolLiteral,

    ArrayIntegerLiteral,
    ArrayDoubleLiteral,
    ArrayCharLiteral,
    ArrayBoolLiteral,

    Identifier,

    TypeKeyword,
    IfKeyword,
    ElseKeyword,
    ElifKeyword,
    ForKeyword,
    WhileKeyword,
    ReturnKeyword,
    VoidKeyword,
    ConstKeyword,
    NamespaceKeyword,

    SwitchKeyword,
    CaseKeyword,
    DefaultKeyword,

    BreakKeyword,
    ContinueKeyword,

    Plus,
    Minus,
    Star,
    Slash,

    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,

    Equal,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LogicalAnd,
    LogicalOr,
    LogicalNot,

    LParen,     //()
    RParen,
    LBracket,   //[]
    RBracket,
    LBrace,     //{}
    RBrace,

    Semicolon,  //;
    Colon,      //:
    ColonColon, //::
    Comma,
    EndOfFile,
    Unknown,
};

struct Token {
    TokenType type;
    double numericValue; //valore se il token corrisponde ad un numero
    std::string lexeme;     //testo interpretato come token
    TextPosition position;
};

namespace 
{
    inline std::unordered_map<TokenType, std::string> type_to_string =
    {
        {TokenType::IntegerLiteral, "<integer>"},
        {TokenType::DoubleLiteral, "<double>"},
        {TokenType::CharLiteral, "<char>"},
        {TokenType::BoolLiteral, "<bool>"},
        {TokenType::ArrayIntegerLiteral, "<int[]>"},
        {TokenType::ArrayDoubleLiteral, "<double[]>"},
        {TokenType::ArrayCharLiteral, "<char[]>"},
        {TokenType::ArrayBoolLiteral, "<bool[]>"},
        {TokenType::Identifier, "identifier"},
        {TokenType::TypeKeyword, "type keyword"},
        {TokenType::IfKeyword, "if"},
        {TokenType::ElseKeyword, "else"},
        {TokenType::ElifKeyword, "elif"},
        {TokenType::ForKeyword, "for"},
        {TokenType::WhileKeyword, "while"},
        {TokenType::ReturnKeyword, "return"},
        {TokenType::VoidKeyword, "void"},
        {TokenType::ConstKeyword, "const"},
        {TokenType::NamespaceKeyword, "namespace"},
        {TokenType::SwitchKeyword, "switch"},
        {TokenType::CaseKeyword, "case"},
        {TokenType::DefaultKeyword, "default"},
        {TokenType::BreakKeyword, "break"},
        {TokenType::ContinueKeyword, "continue"},
        {TokenType::Plus, "+"},
        {TokenType::Minus, "-"},
        {TokenType::Star, "*"},
        {TokenType::Slash, "/"},
        {TokenType::PlusEqual, "+="},
        {TokenType::MinusEqual, "-="},
        {TokenType::StarEqual,"*="},
        {TokenType::SlashEqual,"/="},
        {TokenType::Equal,"="},
        {TokenType::EqualEqual,"=="},
        {TokenType::NotEqual,"!="},
        {TokenType::Less,"<"},
        {TokenType::LessEqual,"<="},
        {TokenType::Greater,">"},
        {TokenType::GreaterEqual,">="},
        {TokenType::LogicalAnd,"&&"},
        {TokenType::LogicalOr,"||"},
        {TokenType::LogicalNot,"!"},
        {TokenType::LParen, "("},
        {TokenType::RParen,")"},
        {TokenType::LBracket, "["},
        {TokenType::RBracket,"]"},
        {TokenType::LBrace, "{"},
        {TokenType::RBrace,"}"},
        {TokenType::Semicolon, ";"},
        {TokenType::Colon,  ":"},
        {TokenType::ColonColon, "::"},
        {TokenType::Comma,","},
        {TokenType::EndOfFile,"EOF"},
        {TokenType::Unknown,"unknown"},
    };
}

inline std::string typeToString(TokenType type) {
    try {
        return type_to_string.at(type);
    } 
    catch (std::out_of_range e) {
        return "<error-translation>";
    }
}


#endif // TOKEN_H
