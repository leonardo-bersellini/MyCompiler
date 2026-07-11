#include "errorlog.h"

#include <QDebug>

ErrorLog::ErrorLog() {}

/*
 * Aggiunge un oggetto errore alla lista interna
 */

void ErrorLog::addError(const QString &message, TextPosition position)
{
    CompilerError err;
    err.message = message;
    err.position = position;
    this->errors.append(err);
}

/*
 * Restituisce la lista intern di errori
 */

const QList<CompilerError>& ErrorLog::getErrors() const
{
    return this->errors;
}

/*
 * Restituisce true se il numero di errori è maggiore di 0
 */

bool ErrorLog::hasErrors() const
{
    return (this->errors.size() > 0);
}

/*
 * Scrive in output gli errori accumulati
 */

void ErrorLog::printErrors() const {
    for (const auto& err : errors) {
        qDebug() << "Error at line" << err.position.line
                << "col" << err.position.column
                << ":" << err.message;
    }
}

void ErrorLog::clear() {
    this->errors.clear();
}