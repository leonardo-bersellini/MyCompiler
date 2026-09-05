#pragma once

#include <string>
#include <memory>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semanticanalyzer.h"
#include "errors/errorlog.h"
#include "AbstractSintaxTree.h"

inline std::vector<Token> lexSource(const std::string& source, ErrorLog& errorLog)
{
    Lexer lexer;
    return lexer.analiseString(source, errorLog);
}

inline std::unique_ptr<Program> parseSource(const std::string& source, ErrorLog& errorLog)
{
    auto tokens = lexSource(source, errorLog);
    Parser parser;
    return parser.parseProgram(tokens, errorLog);
}

inline std::unique_ptr<Program> analyzeSource(const std::string& source, ErrorLog& errorLog)
{
    auto program = parseSource(source, errorLog);
    SemanticAnalyzer analyzer;
    analyzer.analyzeProgram(*program, errorLog);
    return program;
}
