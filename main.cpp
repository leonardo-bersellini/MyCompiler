#include <QCoreApplication>

#include <QFile>
#include <iostream>
#include <memory>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semanticanalyzer.h"
#include "errorlog.h"

#include "codegen/codegenerator.h"

int main(int argc, char *argv[])
{
    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    ErrorLog log;

    QString program_string; //contenuto del file programma

    // STRING_GET, FILE

    QString filepath = argv[1];
    if(filepath.isEmpty()) {
        std::cout << "Inserire filepath per runnare il compiler: ";
        std::string temp;
        std::cin >> temp;
        filepath = QString::fromStdString(temp);
    }

    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly)){
        std::cout << "Unable to open the file";
        return -1;
    }

    // STRING ELABORATION TO PROGRAM

    program_string = QString::fromUtf8(file.readAll().constData());
    std::cout << "\nText:" << program_string.toStdString() << std::endl;

    auto tokens = lexer.analiseString(program_string, log);
    auto program = parser.parseProgram(tokens, log);
    analyzer.analyzeProgram(*program.get(), log);

    std::cout << "\nProgram statements:\n" << std::endl;
    for (const auto& stmt : program->statements) {
        printStmt(stmt.get());
    }

    if (log.hasErrors()) {
        log.printErrors();
    } else {
        std::cout << "\nNo errors found\n" << std::endl;
        std::cout << "Building obj target...\n" << std::endl;

        CodeGenerator codegen;
        codegen.generate(*program.get());
        codegen.emitIR();
        codegen.buildTarget();
    }
    log.clear();

    return 0;
}
