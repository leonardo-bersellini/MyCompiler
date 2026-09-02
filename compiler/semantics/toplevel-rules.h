#ifndef TOPLEVEL_RULES_H
#define TOPLEVEL_RULES_H

#include <array>
#include <ranges>
#include <algorithm>
#include <typeindex>
#include <typeinfo>

#include "AbstractSintaxTree.h"

inline const std::array<std::type_index, 1> valid_global_stmts = 
{
    // typeid( DeclarationStmt ), -> bisogna modificare il codegen per supportare alloca globali
    typeid( FunctionStmt    ),
};

inline const std::array<std::string, 5> valid_winmain_identifiers = 
{
    "WinMain", "winmain", "winMain",
    "Main", "main"
};

bool isValidAtTopLevel(const Stmt& s) {
    return std::ranges::find(valid_global_stmts, typeid(s)) != valid_global_stmts.end();
}

bool isWinMain(const Stmt* st) {
    if(auto s = dynamic_cast<const FunctionStmt*>(st)) 
    {
        if(auto* v = std::get_if<PrimitiveType>(&s->returnType.category)) {
            if(*v != PrimitiveType::Int) return false;
        }
        else return false;

        auto found = std::find(valid_winmain_identifiers.begin(), valid_winmain_identifiers.end(), s->name);
        if(found == valid_winmain_identifiers.end()) return false;

        return true;
    }

    return false;
}


#endif //TOPLEVEL_RULES_H