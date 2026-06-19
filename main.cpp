#include <QCoreApplication>

#include <QFile>
#include <iostream>
#include <memory>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semanticanalyzer.h"
#include "errorlog.h"

/*
     * Language Structure
     */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    ErrorLog log;

    QString program_string; //contenuto del file programma

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

    program_string = QString::fromUtf8(file.readAll().constData());
    qDebug() << "text:" << program_string;

    auto tokens = lexer.analiseString(program_string, log);
    auto program = parser.parseProgram(tokens, log);
    analyzer.analyzeProgram(*program.get(), log);

    for (const auto& stmt : program->statements) {
        printStmt(stmt.get());
    }

    if (log.hasErrors()) {
        log.printErrors();
    } else {
        qDebug() << "no errors found";
    }
    log.clear();

    return QCoreApplication::exec();
}
