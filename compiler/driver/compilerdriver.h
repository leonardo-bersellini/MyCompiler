#ifndef COMPILERDRIVER_H
#define COMPILERDRIVER_H

#include <QCommandLineParser>
#include <string>
#include <optional>

#include "options.h"
#include "flags.h"

struct ParsedFlag {
    std::string name;
    std::optional<std::string> value;
    bool isPipelineFlag; // per sapere in Fase B quale tabella ricontrollare
};

class CompilerDriver
{
public:
    int run(const QCoreApplication &app);

private:
    //CompilerOptions options;
    std::vector<ParsedFlag> parsedFlags;

    QCommandLineParser parser;
    void initCommandLineParser();

    bool parseArguments(const QCoreApplication &app, CompilerOptions &options);
    bool validateOptions(CompilerOptions &options);
    int execute(const CompilerOptions &options);

    void printUsage() const;
    void reportCliError(const QString &message) const;
    void reportCliMsg(const QString &message) const;

    bool compilePipeline(const QString& source, const CompilerOptions &options);
};

#endif // COMPILERDRIVER_H
