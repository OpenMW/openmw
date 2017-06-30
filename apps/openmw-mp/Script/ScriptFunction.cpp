//
// Created by koncord on 23.01.16.
//

#include<iostream>
#include <stdexcept>
#include "ScriptFunction.hpp"

#if !defined(_WIN32) && !defined(__ARM_ARCH) // temporarily disabled
#include <call.hpp>
#endif

#if defined (ENABLE_LUA)
#include "LangLua/LangLua.hpp"
#endif

#if defined (ENABLE_PAWN)
#include "LangPawn/LangPAWN.hpp"
#endif

using namespace std;

ScriptFunction::ScriptFunction(ScriptFunc fCpp,char ret_type, const string &def) :
        fCpp(fCpp), ret_type(ret_type), def(def), script_type(SCRIPT_CPP)
{

}
#if defined (ENABLE_LUA)
ScriptFunction::ScriptFunction(const ScriptFuncLua &fLua, lua_State *lua, char ret_type, const std::string &def) :
        fLua({lua, fLua}), ret_type(ret_type), def(def), script_type(SCRIPT_LUA)
{

}
#endif

#if defined (ENABLE_PAWN)
ScriptFunction::ScriptFunction(const ScriptFuncPAWN &fPawn, AMX *amx, char ret_type, const string &def) :
        fPawn({amx, fPawn}), def(def), ret_type(ret_type), script_type(SCRIPT_PAWN)
{

}
#endif


ScriptFunction::~ScriptFunction()
{
#if defined (ENABLE_PAWN)
    if (script_type == SCRIPT_PAWN)
        fPawn.name.~ScriptFuncPAWN();
#if defined (ENABLE_LUA)
    else
#endif
#endif
#if defined (ENABLE_LUA)
    if (script_type == SCRIPT_LUA)
        fLua.name.~ScriptFuncLua();
#endif
}

boost::any ScriptFunction::Call(const vector<boost::any> &args)
{
    boost::any result;

    if (def.length() != args.size())
        throw runtime_error("Script call: Number of arguments does not match definition");
#if defined (ENABLE_PAWN)
    if (script_type == SCRIPT_PAWN)
    {
        LangPAWN langPawn(fPawn.amx);
        boost::any any = langPawn.Call(fPawn.name.c_str(), def.c_str(), args);
        result = boost::any();

        cell ret = boost::any_cast<cell>(any);

        switch (ret_type)
        {
            case 'i':
                result = static_cast<unsigned int>(ret);
                break;
            case 'q':
                result = static_cast<signed int>(ret);
                break;
            case 's':
                throw runtime_error("Pawn call: the Pawn does not supported strings in public functions");
            case 'f':

                result =  static_cast<double>(amx_ctof(ret));
                break;
            case 'v':
                result = boost::any();
                break;
            default:
                throw runtime_error("Pawn call: Unknown return type" + ret_type);
        }
    }
#endif
#if defined (ENABLE_LUA)
    else if (script_type == SCRIPT_LUA)
    {
        LangLua langLua(fLua.lua);
        boost::any any = langLua.Call(fLua.name.c_str(), def.c_str(), args);

        switch (ret_type)
        {
            case 'i':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<unsigned int>();
                break;
            case 'q':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<signed int>();
                break;
            case 'f':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<double>();
                break;
            case 's':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<const char*>();
                break;
            case 'v':
                result = boost::any();
                break;
            default:
                throw runtime_error("Lua call: Unknown return type" + ret_type);
        }
    }
#endif
    else
    {
        string::iterator it;
        vector<boost::any>::const_iterator it2;
        vector<intptr_t> data;
        CallArgs callArgs;

        for (it = def.begin(), it2 = args.begin(); it != def.end(); ++it, ++it2)
        {
            switch (*it)
            {
                case 'i':
                    callArgs.push_integer(boost::any_cast<unsigned int>(*it2));
                    break;
                case 'q':
                    callArgs.push_integer(boost::any_cast<signed int>(*it2));
                    break;
                case 'f':
                    callArgs.push_double(boost::any_cast<double>(*it2));
                    break;
                case 'd':
                    callArgs.push_double(boost::any_cast<double*>(*it2));
                    break;
                case 's':
                    callArgs.push_stringPtr(boost::any_cast<const char *>(*it2));
                    break;
                case 'v':
                    result = boost::any();
                    break;
                default:
                    throw runtime_error("C++ call: Unknown argument identifier " + *it);
            }
        }
#if !defined(_WIN32) && !defined(__ARM_ARCH) // temporarily disabled
        Func f = reinterpret_cast<Func>(fCpp);
        result = ::Call(f, callArgs);
#else
        throw runtime_error("C++ call: Windows and ARM not supported yet.")
#endif
    }

    return result;
}
