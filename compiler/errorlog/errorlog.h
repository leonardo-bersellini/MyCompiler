#ifndef ERRORLOG_H
#define ERRORLOG_H

#include <string>
#include <vector>
#include "token.h" //TextPosition

struct CompilerError {
    std::string message;
    TextPosition position;
};

class ErrorLog
{
public:
    ErrorLog();

    void addError(const std::string& message, TextPosition position = {-1,-1});
    bool hasErrors() const;
    const std::vector<CompilerError>& getErrors() const;
    void printErrors() const;
    void clear();

private:
    std::vector<CompilerError> errors;
};

#endif // ERRORLOG_H
