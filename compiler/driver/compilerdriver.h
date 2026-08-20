#ifndef COMPILERDRIVER_H
#define COMPILERDRIVER_H

#include <string>
#include <optional>

#include "options.h"
#include "flags.h"
#include "commandlineparser/commandlineparser.h"

struct ParsedFlag {
    std::string name;
    std::optional<std::string> value;
    bool isPipelineFlag; // per sapere in Fase B quale tabella ricontrollare
};

class CompilerDriver
{
public:
    int run(int argc, char* argv[]);

private:
    //CompilerOptions options;
    std::vector<ParsedFlag> parsedFlags;

    CommandLineParser parser;
    void initCommandLineParser();

    bool parseArguments(int argc, char* argv[], CompilerOptions &options);
    bool validateOptions(CompilerOptions &options);
    int execute(const CompilerOptions &options);

    void printUsage() const;
    void reportCliError(const std::string &message) const;
    void reportCliMsg(const std::string &message) const;

    bool compilePipeline(const std::string& source, const CompilerOptions &options);
};

#endif // COMPILERDRIVER_H
