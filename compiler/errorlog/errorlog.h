#ifndef ERRORLOG_H
#define ERRORLOG_H

#include <QString>
#include <QList>
#include "token.h" //TextPosition

struct CompilerError {
    QString message;
    TextPosition position;
};

class ErrorLog
{
public:
    ErrorLog();

    void addError(const QString& message, TextPosition position = {-1,-1});
    bool hasErrors() const;
    const QList<CompilerError>& getErrors() const;
    void printErrors() const;
    void clear();

private:
    QList<CompilerError> errors;
};

#endif // ERRORLOG_H
