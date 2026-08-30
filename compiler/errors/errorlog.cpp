#include "errorlog.h"

#include <iostream>
#include <algorithm>

ErrorLog::ErrorLog() {}

/*
 * Aggiunge un oggetto errore alla lista interna
 */

void ErrorLog::addError(const std::string &message, TextPosition position)
{
    LogEntry err;
    err.type = LogType::Error;
    err.message = message;
    err.position = position;

    addEntry(err);
}

/*
 * Aggiunge un warning alla lista interna
 */

void ErrorLog::addWarning(const std::string& message)
{
    LogEntry w;
    w.type = LogType::Warning;
    w.message = message;
} 

/*
 * Aggiunge un entry generica agli elementi del log
 */

void ErrorLog::addEntry(LogEntry& entry) 
{
    this->entries.push_back(entry);
}

/*
 * Restituisce true se il numero di errori è maggiore di 0
 */

bool ErrorLog::hasEntries() const {
    return this->entries.size() > 0;
}

bool ErrorLog::hasErrors() const
{
    return std::any_of(entries.begin(), entries.end(), 
                    [](const LogEntry& e) {return e.type == LogType::Error;});
}

bool ErrorLog::hasWarnings() const  
{
    return std::any_of(entries.begin(), entries.end(), 
                    [](const LogEntry& e) {return e.type == LogType::Warning;});
}

/*
 * Le funzioni di print scrivono in output gli elementi accumulati
 */

void ErrorLog::printErrors() const {
    for (const auto& e : entries) 
    {
        if(e.type != LogType::Error) continue;

        std::cout << "Error at line " << e.position.line << " col " << e.position.column 
                  << ": " << e.message << std::endl;
    }
}

void ErrorLog::printWarnings() const {
    for(const auto& e : entries) 
    {
        if(e.type != LogType::Warning) continue;

        std::cout << "Warning: " << e.message << std::endl;
    }
}

void ErrorLog::printAll() const 
{
    for(const auto& e : entries)
    {
        if(e.type == LogType::Error) {
                std::cout << "Error at line " << e.position.line << " col " << e.position.column 
                          << ": " << e.message << std::endl;
        }
        else if(e.type == LogType::Warning) {
            std::cout << "Warning: " << e.message << std::endl;
        }
    }
}

void ErrorLog::clear() {
    this->entries.clear();
}