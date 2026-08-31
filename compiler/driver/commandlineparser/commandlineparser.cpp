#include "commandlineparser.h"

#include <iostream>
#include <cstdlib>
#include <algorithm>

void CommandLineParser::setApplicationDescription(const std::string& description)
{
    applicationDescription = description;
}

void CommandLineParser::addVersionOption(const std::string& version)
{
    applicationVersion = version;
    versionOptionAdded = true;
}

void CommandLineParser::addHelpOption()
{
    helpOptionAdded = true;
}

void CommandLineParser::addPositionalArgument(const std::string& name, const std::string& description)
{
    positionalArgumentDefs.push_back({name, description});
}

void CommandLineParser::addOption(const CommandLineOption& option)
{
    std::size_t index = options.size();
    options.push_back(option);

    for (const std::string& alias : option.names) {
        nameToOptionIndex[alias] = index;
    }
}

void CommandLineParser::reportError(const std::string& message) const
{
    std::cerr << "error: " << message << std::endl;
}

void CommandLineParser::printHelp() const
{
    if (!applicationDescription.empty()) {
        std::cout << applicationDescription << "\n" << std::endl;
    }

    std::cout << "Usage:";
    for (const PositionalArgument& pos : positionalArgumentDefs) {
        std::cout << " <" << pos.name << ">";
    }
    if (!options.empty() || helpOptionAdded || versionOptionAdded) {
        std::cout << " [options]";
    }
    std::cout << "\n" << std::endl;

    if (!positionalArgumentDefs.empty()) {
        std::cout << "Arguments:\n";
        for (const PositionalArgument& pos : positionalArgumentDefs) {
            std::cout << "  " << pos.name << "\t" << pos.description << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "Options:\n";

    if (helpOptionAdded) {
        std::cout << "  -h, --help\tMostra questo messaggio di aiuto\n";
    }
    if (versionOptionAdded) {
        std::cout << "  -v, --version\tMostra la versione dell'applicazione\n";
    }

    for (const CommandLineOption& opt : options) {
        std::cout << "  ";
        for (std::size_t i = 0; i < opt.names.size(); ++i) 
        {
            std::string prefix = (opt.names[i].size() > short_flag_size) ? "--" : "-";
            std::cout << prefix << opt.names[i];
            if (i + 1 < opt.names.size()) {
                std::cout << ", ";
            }
        }
        if (!opt.valueName.empty()) {
            std::cout << " <" << opt.valueName << ">";
        }
        std::cout << "\t" << opt.description << std::endl;
    }
}

int CommandLineParser::process(int argc, char* argv[])
{
    std::vector<std::string> args(argv + 1, argv + argc);

    std::size_t i = 0;

    while (i < args.size()) {
        const std::string& arg = args[i];

        // rimozione dei prefissi - o --
        std::string name;
        bool isOption = false;

        if (arg.rfind("--", 0) == 0) {
            name = arg.substr(2);
            isOption = true;
        } 
        else if (arg.rfind("-", 0) == 0 && arg.size() > 1) {
            name = arg.substr(1);
            isOption = true;
        }

        if(!isOption) {
            parsedPositionalArguments.push_back(arg);
            ++i;
            continue;
        }

        if(helpOptionAdded && (name == "h" || name == "help")) {
            printHelp();
            return 0;
        }

        if(versionOptionAdded && (name == "v" || name == "version")) {
            std::cout << applicationVersion << std::endl;
            return 0;
        }

        auto it = nameToOptionIndex.find(name);
        if (it == nameToOptionIndex.end()) {
            reportError("unknown option: " + arg);
            return 1;
        }

        const CommandLineOption& opt = options[it->second];
        const std::string& canonicalName = opt.names.front();

        if (!opt.valueName.empty()) {
            if (i + 1 >= args.size()) {
                reportError("missing value for option: " + arg);
                std::exit(1);
            }
            parsedValues[canonicalName] = args[i + 1];
            i += 2;
        } else {
            parsedValues[canonicalName] = "";
            ++i;
        }
    }

    // applicazione dei default per le opzioni con valore non impostate
    for (const CommandLineOption& opt : options) 
    {
        const std::string& canonicalName = opt.names.front();

        if (!opt.valueName.empty() && parsedValues.find(canonicalName) == parsedValues.end()
            && !opt.defaultValue.empty()) {
            parsedValues[canonicalName] = opt.defaultValue;
        }
    }
}

bool CommandLineParser::isSet(const std::string& name) const
{
    auto it = nameToOptionIndex.find(name);
    if (it == nameToOptionIndex.end()) {
        return false;
    }
    const std::string& canonicalName = options[it->second].names.front();
    return parsedValues.find(canonicalName) != parsedValues.end();
}

std::string CommandLineParser::value(const std::string& name) const
{
    auto it = nameToOptionIndex.find(name);
    if (it == nameToOptionIndex.end()) {
        return "";
    }
    const std::string& canonicalName = options[it->second].names.front();
    auto valIt = parsedValues.find(canonicalName);
    return (valIt != parsedValues.end()) ? valIt->second : "";
}

std::vector<std::string> CommandLineParser::positionalArguments() const
{
    return parsedPositionalArguments;
}