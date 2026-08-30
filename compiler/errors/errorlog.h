#ifndef ERRORLOG_H
#define ERRORLOG_H

#include <string>
#include <vector>
#include "token.h" //TextPosition

enum class LogType 
{
    Error,
    Warning
};

struct LogEntry 
{
    LogEntry() = default;

    LogType type;
    std::string message;
    TextPosition position; //solo per errors
};

class ErrorLog
{
public:
    ErrorLog();

    void addError(const std::string& message, TextPosition position = {-1,-1});
    void addWarning(const std::string& message);

    bool hasErrors() const;
    void printErrors() const;

    bool hasWarnings() const;
    void printWarnings() const;

    bool hasEntries() const;
    void printAll() const;
    
    void clear();

private:
    std::vector<LogEntry> entries;

    void addEntry(LogEntry& entry);
};

#endif // ERRORLOG_H
