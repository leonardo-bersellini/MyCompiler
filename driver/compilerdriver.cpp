#include "compilerdriver.h"

#include <QFile>
#include <QCommandLineParser>
#include <iostream>
#include <memory>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semanticanalyzer.h"
#include "errorlog.h"
#include "codegen/codegenerator.h"

/**
 * COMPILER DRIVER
 * Questa classe rappresenta il driver di gestione del compilatore da shell di comando (CLI).
 * Il driver ha la funzione di richiamare la pipeline corretta in base al comando selezionato
 * dall'utente da riga di comando.
 */

/*
 * Pipeline del driver:
 * run -> parse -> validate -> execute -> report errors / exitcode
 *
 * La funzione di run è il metodo pubblico che funge da entrypoint per
 * il driver da terminale.
 */

int CompilerDriver::run(const QCoreApplication &app)
{
    initCommandLineParser();

    CompilerOptions options;
    if (!parseArguments(app, options)) {
        return 1; // errore già stampato da reportCliError
    }

    if (!validateOptions(options)) {
        return 1;
    }

    return execute(options);
}

/*
 * Questa funzione si occupa di inizializzare la classe qt QCommandLineParser,
 * (utilizzata come parser degli argomenti a riga di comando), con i flag e gli argomenti
 * posizionali conservati nelle tabelle delle flag, in flags.h
 */

void CompilerDriver::initCommandLineParser()
{
    parser.setApplicationDescription("Compilatore sviluppato in C++ e LLVM");
    parser.addVersionOption();

    //argomento posizionale: file di input
    parser.addPositionalArgument("inputfile", "indirizzo del codice sorgente da compilare");

    for(const PipelineFlag& flag : pipelineFlags)
    {
        if(flag.requiresValue) {
            QCommandLineOption option(flag.name, flag.description, "value");
            //il terzo parametro indica il nome placeholder mostrato nell'helper
            //la sola presenza di questo parametro inizializzato indica che il comando necessita di un valore
            parser.addOption(option);
        } else {
            QCommandLineOption option(flag.name, flag.description);
            parser.addOption(option);
        }
    }

    for(const UtilityFlag& flag : utilityFlags)
    {
        QCommandLineOption option(flag.name, flag.description);
        parser.addOption(option);
    }

    parser.addHelpOption();
}

/*
 * Questa funzione sfrutta il parser inizializzato precedentemente per controllare gli
 * argomenti a riga di comando e, in base ad essi, ritornare un errore o meno.
 */

bool CompilerDriver::parseArguments(const QCoreApplication &app, CompilerOptions &options)
{
    // Parsing degli argomenti
    parser.process(app);

    // Recupero file di input
    const QStringList positionalArgs = parser.positionalArguments();
    if (positionalArgs.isEmpty()) {
        reportCliError("missing input file");
        return false;
    }
    options.inputFile = positionalArgs.first();

    // Pipeline flags, solo uno possibile per volta
    bool pipelineFlagAlreadySet = false;

    for(const PipelineFlag& flag : pipelineFlags)
    {
        if (!parser.isSet(flag.name)) {
            continue;
        }

        if (pipelineFlagAlreadySet) {
            reportCliError("conflicting pipeline flags specified");
            return false;
        }
        pipelineFlagAlreadySet = true;

        options.outkind = flag.resultingKind;

        if (flag.requiresValue && flag.valueTarget != nullptr) {
            options.*(flag.valueTarget) = parser.value(flag.name);
        }
    }

    // Utility flags: nessun vincolo, ognuno indipendente
    for (const UtilityFlag &flag : utilityFlags) {
        options.*(flag.target) = parser.isSet(flag.name);
    }

    return true;
}

/*
 * Questa funzione si occupa di eseguire il controllo di correttezza delle opzioni
 * ricostruite dal parsing.
 * Se non sono riscontarti errori negli argomenti selezionati, si può procedere con l'esecuzione.
 */

bool CompilerDriver::validateOptions(const CompilerOptions &options)
{
    QFile inputFile(options.inputFile);
    if (!inputFile.exists()) {
        reportCliError("input file does not exist: " + options.inputFile);
        return false;
    }

    switch (options.outkind)
    {
    case OutputKind::ObjectFile:
        if (options.outputFile.isEmpty()) {
            reportCliError("missing output file (-o)");
            return false;
        }
        break;
    case OutputKind::Executable:
        reportCliError("this output kind is not implemented yet");
        return false;
    }

    return true;
}

/*
 * Questa funzione si occuapa dell'esecuzione vera e propria della pipeline selezionata da
 * shell di comando.
 * In base alle opzioni costruite nei passaggi precedenti, si invocano funzioni helper per
 * eseguire una pipeline specifica.
 */

int CompilerDriver::execute(const CompilerOptions &options)
{
    QFile file(options.inputFile);
    if (!file.open(QIODevice::ReadOnly)) {
        reportCliError("unable to open input file: " + options.inputFile);
        return 1;
    }

    QString sourceCode = file.readAll().constData();

    // Pipeline di compilazione
    bool good = compilePipeline(sourceCode, options);

    if(!good) {
        reportCliError("collected -1 exit status");
    } else {
        std::cout << "\nexecution returned with exit code 0" << std::endl;
    }

    return 0;
}

/*
 * Metodo helper per ritornare errori a console.
 */

void CompilerDriver::reportCliError(const QString &message) const
{
    std::cerr << "error: " << message.toStdString() << std::endl;
}

/*
 * Questa funzione è il cuore del driver di compilazione.
 * Si occupa di esrguire i passaggi di compilazione in modo che la pipeline selezionata
 * sia rispettata, gestendo le componenti di compilazione del progetto.
 */

bool CompilerDriver::compilePipeline(const QString &source, const CompilerOptions &options)
{
    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    ErrorLog errorLog;
    CodeGenerator codegen;

    // Lettura e parsing del codice, indipendente dai flags
    auto tokens = lexer.analiseString(source, errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("lexer execution return error code: typo error");
        return false;
    }

    auto program = parser.parseProgram(tokens, errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("parser execution return error code: syntax error");
        return false;
    }

    analyzer.analyzeProgram(*program.get(), errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("analyzer execution return error code: semantic error");
        return false;
    }

    // Flag aggiuntivi per output a console

    if(options.emitIR)
    {
        codegen.emitIR();
    }
    else if(options.verbose)
    {
        std::cout << "\nSource text:" << source.toStdString() << std::endl;

        lexer.printTokens();

        std::cout << "\nProgram statements:\n" << std::endl;
        for (const auto& stmt : program->statements) {
            printStmt(stmt.get());
        }

        std::cout << "\nNo errors found\n" << std::endl;
        std::cout << "Building obj target...\n" << std::endl;

        codegen.emitIR();
    }

    // Opzioni di output kind

    QFile out(options.outputFile);
    if(!out.open(QIODevice::WriteOnly)) {
        reportCliError("unable to create and open output file: " + options.outputFile);
        return false;
    }
    out.close();

    switch(options.outkind)
    {
    case OutputKind::Executable :
        reportCliError("executable output kind not implemented yet");
        return false;

    case OutputKind::ObjectFile :
        codegen.generate(*program.get());
        codegen.buildTargetObj(options.outputFile);
        // TODO modificare e dividere in sottofunzioni buildtargetobj, aggiungere opzioni di
        // output condizionale per debug (--build-debug)
    }

    return true;
}





