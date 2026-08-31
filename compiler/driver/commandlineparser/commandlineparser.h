#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <string>
#include <vector>
#include <unordered_map>

struct CommandLineOption
{
	CommandLineOption() = default;

    CommandLineOption(const std::string& name,
                       const std::string& description,
                       const std::string& valueName = "")
        : names{name}, description(description), valueName(valueName)
    {}

    CommandLineOption(const std::vector<std::string>& names,
                       const std::string& description,
                       const std::string& valueName = "")
        : names(names), description(description), valueName(valueName)
    {}
	
    std::vector<std::string> names;
    std::string description;
    std::string valueName;      // vuoto = opzione senza valore
    std::string defaultValue;   // default ""
};

struct PositionalArgument
{
    std::string name;
    std::string description;
};

class CommandLineParser
{
public:
    void setApplicationDescription(const std::string& description);
    void addVersionOption(const std::string& version);
    void addHelpOption();
    void addPositionalArgument(const std::string& name, const std::string& description);
    void addOption(const CommandLineOption& option);

    int process(int argc, char* argv[]);

    bool isSet(const std::string& name) const;
    std::string value(const std::string& name) const;
    std::vector<std::string> positionalArguments() const;

private:
    std::string applicationDescription;
    std::string applicationVersion;
    bool versionOptionAdded = false;
    bool helpOptionAdded = false;
    const int short_flag_size = 2; //sotto N caratteri i flag sono short

    std::vector<CommandLineOption> options;
    std::unordered_map<std::string, std::size_t> nameToOptionIndex; 

    std::vector<PositionalArgument> positionalArgumentDefs;
    std::vector<std::string> parsedPositionalArguments;

    std::unordered_map<std::string, std::string> parsedValues; 

    void printHelp() const;
    void reportError(const std::string& message) const;
};

#endif // COMMANDLINEPARSER_H