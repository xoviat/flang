
#include "MSVC.h"

#include <string>

using namespace flang::platform;

std::vector<std::string> MSVC::GetFlangLibsLinkerArgs(std::vector<std::string> libs)
{
    std::vector<std::string> base = {
        "msvcrt",
        "kernel32",
        "user32",
        "gdi32",
        "winspool",
        "shell32",
        "ole32",
        "oleaut32",
        "uuid",
        "comdlg32",
        "advapi32",
    };

    std::vector<std::string> v;
    for (auto lib : base)
    {
        v.push_back("/defaultlib:" + lib);
    }

    for (auto lib : libs)
    {
        v.push_back("/defaultlib:" + lib);
    }

    return v;
}

std::vector<std::string> MSVC::GetMainLinkerArgs(std::vector<std::string> libs)
{
    std::vector<std::string> v = {
        "/subsystem:console",
    };

    for (auto lib : libs)
    {
        v.push_back("/defaultlib:" + lib);
    }

    return v;
}

std::vector<std::string> MSVC::GetCompileDefinitions()
{
    return {
        "__LONG_MAX__=2147483647L",
        "__SIZE_TYPE__=unsigned long long int",
        "__PTRDIFF_TYPE__=long long int",
        "_WIN32",
        "WIN32",
        "_WIN64",
        "WIN64",
        "_MSC_VER=" + std::to_string(_MSC_VER)};
}