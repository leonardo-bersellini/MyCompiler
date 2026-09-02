#ifndef ANSI_H
#define ANSI_H


#include <string>
#include <iostream>

namespace ansi 
{
    void enableAnsi();

    namespace color {
        inline constexpr std::string reset   = "\033[0m";
        inline constexpr std::string black   = "\033[30m";
        inline constexpr std::string red     = "\033[31m";
        inline constexpr std::string green   = "\033[32m";
        inline constexpr std::string yellow  = "\033[33m";
        inline constexpr std::string blue    = "\033[34m";
        inline constexpr std::string magenta = "\033[35m";
        inline constexpr std::string cyan    = "\033[36m";
        inline constexpr std::string white   = "\033[37m";

        inline constexpr std::string bright_black = "\033[90m";
        inline constexpr std::string bright_yellow = "\033[93m";
    }
}


#endif //ANSI_H