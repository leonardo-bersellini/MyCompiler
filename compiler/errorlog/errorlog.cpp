#include "errorlog.h"

#include <iostream>

ErrorLog::ErrorLog() {}

/*
 * Aggiunge un oggetto errore alla lista interna
 */

void ErrorLog::addError(const std::string &message, TextPosition position)
{
    CompilerError err;
    err.message = message;
    err.position = position;
    this->errors.push_back(err);
}

/*
 * Restituisce la lista intern di errori
 */

const std::vector<CompilerError>& ErrorLog::getErrors() const
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
        std::cout << "Error at line" << err.position.line
                << "col" << err.position.column
                << ":" << err.message << std::endl;
    }
}

void ErrorLog::clear() {
    this->errors.clear();
}