//
// Created by koncord on 09.05.16.
//

#ifndef _WIN32
    #include <dlfcn.h>
#endif

#include <stdexcept>
#include "LangNative.hpp"
#include <Script/SystemInterface.hpp>
#include <Script/Script.hpp>

using namespace std;

template<typename R>
bool SetScript(lib_t lib, const char *name, R value)
{
    SystemInterface<R *> result(lib, name);

    if (result)
        *result.result = value;

    return result.operator bool();
}

void LangNative::LoadProgram(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file)
        throw runtime_error("Script not found: " + string(filename));

    fclose(file);

#ifdef _WIN32
    lib = LoadLibrary(filename);
#else
    lib = dlopen(filename, RTLD_LAZY);
#endif

    if (!lib)
        throw runtime_error("Was not able to load C++ script: " + string(filename));

    try
    {

        const char *prefix = SystemInterface<const char *>(lib, "prefix").result;
        string pf(prefix);

        for (const auto &function : ScriptFunctions::functions)
            if (!SetScript(lib, string(pf + function.name).c_str(), function.func.addr))
                LOG_MESSAGE_SIMPLE(Log::LOG_WARN, "Script function pointer not found: %s", function.name);
    }
    catch (...)
    {
        FreeProgram();
        throw;
    }
}

int LangNative::FreeProgram()
{
#ifdef _WIN32
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
    return 0;
}

bool LangNative::IsCallbackPresent(const char *name)
{
    return true;
}

boost::any LangNative::Call(const char *name, const char *argl, int buf, ...)
{
    return nullptr;
}

boost::any LangNative::Call(const char *name, const char *argl, const std::vector<boost::any> &args)
{
    return nullptr;
}


lib_t LangNative::GetInterface()
{
    return lib;
}


LangNative::LangNative()
{

}

LangNative::~LangNative()
{

}
