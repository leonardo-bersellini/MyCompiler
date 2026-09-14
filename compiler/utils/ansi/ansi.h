#ifndef ANSI_H
#define ANSI_H

#include <string>
#include <iostream>

namespace ansi 
{
    inline bool ansi_enabled = false;

    void enableAnsi();

    void disableAnsi();

    namespace color 
    {
        namespace 
        {
            struct Color {
            public:
                Color() = delete;
                Color(const std::string& code) : ansi_code(code) {}

                std::string getAnsiCode() const {
                    return ansi_enabled ? this->ansi_code : "";
                }

                friend std::ostream& operator<<(const std::ostream& os, const Color& c);
                friend std::string operator+(const std::string& str, const Color& c);

                std::string operator+(const Color& other) const {
                    return this->getAnsiCode() + other.getAnsiCode();
                }
                std::string operator+(const std::string& str) {
                    return this->getAnsiCode() + str;
                }
                

            private:
                std::string ansi_code;
            };

            std::ostream& operator<<(std::ostream& os, const Color& c) {
                os << c.getAnsiCode();
                return os;
            }
            std::string operator+(const std::string& str, const Color& c) {
                return str + c.getAnsiCode();
            }

        
            inline constexpr std::string _reset   = "\033[0m";
            inline constexpr std::string _black   = "\033[30m";
            inline constexpr std::string _red     = "\033[31m";
            inline constexpr std::string _green   = "\033[32m";
            inline constexpr std::string _yellow  = "\033[33m";
            inline constexpr std::string _blue    = "\033[34m";
            inline constexpr std::string _magenta = "\033[35m";
            inline constexpr std::string _cyan    = "\033[36m";
            inline constexpr std::string _white   = "\033[37m";

            inline constexpr std::string _bright_black = "\033[90m";
            inline constexpr std::string _bright_yellow = "\033[93m";
        }

        inline Color reset(_reset);
        inline Color black(_black);
        inline Color red(_red);
        inline Color green(_green);
        inline Color yellow(_yellow);
        inline Color blue(_blue);
        inline Color magenta(_magenta);
        inline Color cyan(_cyan);
        inline Color white(_white);

        inline Color bright_black(_bright_black);
        inline Color bright_yellow(_bright_yellow);
    }
}


#endif //ANSI_H