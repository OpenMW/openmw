//
// Created by koncord on 19.03.16.
//

#include "Script.hpp"
#include "LangNative/LangNative.hpp"

#if defined (ENABLE_PAWN)
#include "LangPawn/LangPAWN.hpp"
#endif
#if defined (ENABLE_LUA)
#include "LangLua/LangLua.hpp"
#endif

using namespace std;

Script::ScriptList Script::scripts;

Script::Script(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (!file)
        throw runtime_error("Script not found: " + string(path));

    fclose(file);

#ifdef _WIN32
    if (strstr(path, ".dll"))
#else
    if (strstr(path, ".so"))
#endif
    {
        script_type = SCRIPT_CPP;
        lang = new LangNative();
    }
#if defined (ENABLE_PAWN)
    else if (strstr(path, ".amx"))
    {
        lang = new LangPAWN();
        script_type = SCRIPT_PAWN;
    }
#endif
#if defined (ENABLE_LUA)
    else if (strstr(path, ".lua") || strstr(path, ".t"))
    {
        lang = new LangLua();
        script_type = SCRIPT_LUA;
    }
#endif
    else
        throw runtime_error("Script type not recognized: " + string(path));

    try
    {
        lang->LoadProgram(path);
    }
    catch (...)
    {
        lang->FreeProgram();
        throw;
    }

}


Script::~Script()
{
    lang->FreeProgram();

    delete lang;
}

void Script::LoadScripts(char *scripts, const char *base)
{
    char *token = strtok(scripts, ",");

    try
    {
        while (token)
        {
            char path[4096];
            snprintf(path, sizeof(path), Utils::convertPath("%s/%s/%s").c_str(), base, "scripts", token);
            Script::scripts.emplace_back(new Script(path));
            token = strtok(nullptr, ",");
        }
    }
    catch (...)
    {
        UnloadScripts();
        throw;
    }
}

void Script::UnloadScripts()
{
    //Public::DeleteAll();
    scripts.clear();
#if defined (ENABLE_TERRA)
    terra_llvmshutdown();
#endif
}

void Script::LoadScript(const char *script, const char *base)
{
    char path[4096];
    snprintf(path, sizeof(path), Utils::convertPath("%s/%s/%s").c_str(), base, "scripts", script);
    Script::scripts.emplace_back(new Script(path));
}
