#include "compilerdriver.h"

#include <iostream>
#include <memory>
#include <fstream>
#include <filesystem>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semanticanalyzer.h"
#include "errorlog/errorlog.h"
#include "codegen/codegenerator.h"

#include "commandlineparser/commandlineparser.h"
#include "utils/ansi.h"

#include "version.h" //generato da cmake

namespace fs = std::filesystem;
namespace clr = ansi::color;

/**
 * COMPILER DRIVER
 * Questa classe rappresenta il driver di gestione del compilatore da shell di comando (CLI).
 * Il driver ha la funzione di richiamare la pipeline corretta in base al comando selezionato
 * dall'utente da riga di comando.
 */

/*
 * Metodo helper per ritornare errori a console.
 */

void CompilerDriver::reportCliError(const std::string &message) const
{
    std::cerr << ansi::color::red << "error: " << message << ansi::color::reset << std::endl;
}

/*
 * Metodo helper per mostrare messaggi a console
 */

void CompilerDriver::reportCliMsg(const std::string &message) const
{
    std::cout << message << std::endl;
}

/*
 * Pipeline del driver:
 * run -> parse -> validate -> execute -> report errors / exitcode
 *
 * La funzione di run è il metodo pubblico che funge da entrypoint per
 * il driver da terminale.
 */

int CompilerDriver::run(int argc, char* argv[])
{
    ansi::enableAnsi();
    initCommandLineParser();

    CompilerOptions options;
    if (!parseArguments(argc, argv, options)) {
        return 1; // errore già stampato
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
    parser.setApplicationDescription(APP_DESCRIPTION);
    parser.addVersionOption(APP_VERSION);
    parser.addHelpOption();

    //argomento posizionale: file di input
    parser.addPositionalArgument("inputfile", "indirizzo del codice sorgente da compilare");

    for(const PipelineFlag& flag : pipelineFlags)
    {
        if(flag.requiresValue) {
            CommandLineOption option(flag.name, flag.description, "value");
            //il terzo parametro indica il nome placeholder mostrato nell'helper
            //la sola presenza di questo parametro inizializzato indica che il comando necessita di un valore
            parser.addOption(option);
        } else {
            CommandLineOption option(flag.name, flag.description);
            parser.addOption(option);
        }
    }

    for(const UtilityFlag& flag : utilityFlags)
    {
        CommandLineOption option(flag.name, flag.description);
        parser.addOption(option);
    }
}

/*
 * Questa funzione sfrutta il parser inizializzato precedentemente per controllare gli
 * argomenti a riga di comando e, in base ad essi, ritornare un errore o meno.
 */

bool CompilerDriver::parseArguments(int argc, char* argv[], CompilerOptions &options)
{
    // Parsing degli argomenti
    parser.process(argc, argv);

    // Recupero file di input
    const std::vector<std::string> positionalArgs = parser.positionalArguments();

    if (positionalArgs.empty()) {
        reportCliError("missing input file");
        return false;
    }

    options.inputFile = positionalArgs.at(0);

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
 * Funzione helper.
 * Si occupa della deduzione del percorso del file di output quando non specificato.
 */

void deduceOutputFile(CompilerOptions& options, const std::string& outExtension)
{
    fs::path inputPath(options.inputFile);

    fs::path directory = inputPath.parent_path();

    if (directory.empty()) {
        directory = fs::current_path();
    }

    fs::path outputFile = directory / (inputPath.stem().string() + outExtension);

    options.outputFile = outputFile.string();

    return;
}

/*
 * Questa funzione si occupa di eseguire il controllo di correttezza delle opzioni
 * ricostruite dal parsing.
 * Se non sono riscontarti errori negli argomenti selezionati, si può procedere con l'esecuzione.
 */

bool CompilerDriver::validateOptions(CompilerOptions &options)
{
    std::ifstream inputFile(options.inputFile, std::ios::in);
    if (!inputFile.is_open()) {
        reportCliError("input file does not exist: " + options.inputFile);
        inputFile.close();
        return false;
    }
    inputFile.close();

    switch (options.outkind)
    {
    case OutputKind::ObjectFile:
        if (options.outputFile.empty())
        {
            deduceOutputFile(options, std::string(".o"));
            reportCliMsg("deduced output file path: " + options.outputFile);
        }
        break;

    case OutputKind::Executable:
        if(options.outputFile.empty())
        {
            deduceOutputFile(options, std::string(".exe"));
            reportCliMsg("deduced output file path: " + options.outputFile);
        }
        break;
    }

    return true;
}

/*
 * Questa funzione si occupa dell'esecuzione vera e propria della pipeline selezionata da
 * shell di comando.
 * In base alle opzioni costruite nei passaggi precedenti, si invocano funzioni helper per
 * eseguire una pipeline specifica.
 */

int CompilerDriver::execute(const CompilerOptions &options)
{
    std::ifstream file(options.inputFile, std::ios::in);
    if (!file.is_open()) {
        reportCliError("unable to open input file: " + options.inputFile);
        return 1;
    }

    std::string sourceCode;
    std::string line;

    while(std::getline(file, line))
    {
        sourceCode.append(line + "\n");
    }

    // Pipeline di compilazione
    bool good = compilePipeline(sourceCode, options);

    if(!good) {
        reportCliError("collected -1 exit status");
    } else {
        reportCliMsg("\nexecution returned with exit code 0");
    }

    return 0;
}

/*
 * Questa funzione è il cuore del driver di compilazione.
 * Si occupa di esrguire i passaggi di compilazione in modo che la pipeline selezionata
 * sia rispettata, gestendo le componenti di compilazione del progetto.
 */

bool CompilerDriver::compilePipeline(const std::string &source, const CompilerOptions &options)
{
    Lexer lexer;
    Parser parser;
    SemanticAnalyzer analyzer;
    ErrorLog errorLog;
    CodeGenerator codegen;

    // Lettura e parsing del codice, indipendente dai flags
    const std::vector<Token> tokens = lexer.analiseString(source, errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("lexer execution return error code: typo error");

        if(options.verbose) {
            errorLog.printErrors();
        }
        if(options.emitIR) {
            reportCliError("could not solve specified options for compiler execution [code-steps-not-generated]");
        }

        return false;
    }

    auto program = parser.parseProgram(tokens, errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("parser execution return error code: syntax error");

        if(options.verbose) {
            errorLog.printErrors();
        }
        if(options.emitIR) {
            reportCliError("could not solve specified options for compiler execution [code-steps-not-generated]");
        }

        return false;
    }

    analyzer.analyzeProgram(*program.get(), errorLog);

    if(errorLog.hasErrors()) {
        reportCliError("analyzer execution return error code: semantic error");

        if(options.verbose) {
            errorLog.printErrors();
        }
        if(options.emitIR) {
            reportCliError("could not solve specified options for compiler execution [code-steps-not-generated]");
        }

        return false;
    }

    // Generazione degli stmt di codice llvm ir
    codegen.generate(*program.get());

    // Flag aggiuntivi per output a console

    if(options.emitIR)
    {
        codegen.emitIR();
    }
    else if(options.verbose)
    {
        reportCliMsg("\nsource text:\n" + clr::bright_black + source + clr::reset);

        lexer.printTokens();

        reportCliMsg("\nprogram statements:\n");
        for (const auto& stmt : program->statements) {
            printStmt(stmt.get());
        }

        reportCliMsg("\nNo errors found\n");
        reportCliMsg("Building obj target...\n");

        codegen.emitIR();
    }

    // Opzioni di output kind

    if(options.noOutputFile) {
        reportCliMsg("\nskipping output file generation...");
        return true;
    }

    std::ofstream out(options.outputFile, std::ios::out | std::ios::binary);
    if(!out.is_open()) {
        reportCliError("unable to create and open output file: " + options.outputFile);
        return false;
    }
    out.close();

    switch(options.outkind)
    {
    case OutputKind::Executable :
        {
            std::string tempObj = options.outputFile + ".o"; // file oggetto temporaneo
            codegen.buildTargetObj(tempObj, options.verbose);

            bool linked = codegen.link(tempObj, options.outputFile, options.verbose);

            fs::remove(tempObj);

            if (!linked) {
                reportCliError("linker execution failed");
                return false;
            }

            break;
        }
    case OutputKind::ObjectFile :
        codegen.buildTargetObj(options.outputFile, options.verbose);
        break;
    }

    return true;
}





