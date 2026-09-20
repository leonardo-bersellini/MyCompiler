#include "linker.h"

// per utilizzo di processi figli di cmd.exe
#ifdef WIN32 
    #define popen _popen
    #define pclose _pclose
#endif

#include <windows.h>
#include <vector>
#include <filesystem>
#include <iostream>

bool linker::lld_link(const std::string &objFile, const std::string &outputExe, bool debug)
{
    wchar_t wbuf[MAX_PATH];
    GetModuleFileNameW(nullptr, wbuf, MAX_PATH);
    std::filesystem::path appDir = std::filesystem::path(wbuf).parent_path();

    std::string linkerPath = appDir.string() + "/lld-link.exe";
    std::string libDir     = appDir.string() + "/libs";

    //argomenti per lld-link
    std::vector<std::string> args = 
    {
        objFile,
        "-out:" + outputExe,
        "-subsystem:console",
        "-libpath:" + libDir,
        "crt2.o",
        "libmingw32.a",
        "libgcc.a",
        "libgcc_eh.a",
        "libmoldname.a",
        "libmingwex.a",
        "libucrt.a",
        "libadvapi32.a",
        "libshell32.a",
        "libuser32.a",
        "libkernel32.a"
    };

    if(debug) {
        std::cout << "linker path: " << clr::bright_black << linkerPath << clr::reset << std::endl;
        std::cout << "exists: " << std::filesystem::exists(linkerPath) << std::endl;
    }

    std::string cmd = linkerPath;
    for (const auto& a : args) cmd += " " + a;
    cmd += " 2>&1";

    if(debug) std::cout << "executing command: " << clr::bright_black << cmd << clr::reset << std::endl;

    FILE* pipe = popen(cmd.c_str(), "r");
    std::string output;
    char _buf[256];
    while (fgets(_buf, sizeof(_buf), pipe)) output += _buf;
    int exitCode = pclose(pipe);

    if (exitCode != 0) {
        if (debug) std::cout << "\nlinker exit code: " << exitCode << "\n" << output << std::endl;
        return false;
    }

    if(debug) {
        std::cout << "linker: " << clr::bright_black << "lld-link.exe [msys64-ucrt64]\n" << clr::reset << std::endl;
        std::cout << "linker exit code: " << exitCode  << std::endl;
    }

    return true;
}