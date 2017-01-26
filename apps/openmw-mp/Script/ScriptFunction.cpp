//
// Created by koncord on 23.01.16.
//

#include<iostream>
#include <stdexcept>
#include "ScriptFunction.hpp"

#if defined (ENABLE_LUA)
#include "LangLua/LangLua.hpp"
#endif

#if defined (ENABLE_PAWN)
#include "LangPawn/LangPAWN.hpp"
#endif

using namespace std;

ScriptFunction::ScriptFunction(ScriptFunc fCpp,char ret_type, const string &def) : fCpp(fCpp), ret_type(ret_type), def(def), script_type(SCRIPT_CPP)
{

}
#if defined (ENABLE_LUA)
ScriptFunction::ScriptFunction(const ScriptFuncLua &fLua, lua_State *lua, char ret_type, const std::string &def) :fLua({lua, fLua}), ret_type(ret_type), def(def), script_type(SCRIPT_LUA)
{

}
#endif

#if defined (ENABLE_PAWN)
ScriptFunction::ScriptFunction(const ScriptFuncPAWN &fPawn, AMX *amx, char ret_type, const string &def) : fPawn({amx, fPawn}),
                                                                                           def(def), ret_type(ret_type), script_type(SCRIPT_PAWN)
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
        throw runtime_error("Native Call: native calls does not supported yet");
#if 0
#ifdef ARCH_X86
        // cdecl convention
        string::iterator it;
        vector<boost::any>::const_iterator it2;
        vector<unsigned int> data;

        for (it = def.begin(), it2 = args.begin(); it != def.end(); ++it, ++it2)
        {
            switch (*it)
            {
                case 'i':
                {
                    unsigned int value = boost::any_cast<unsigned int>(*it2);
                    data.push_back(value);
                    break;
                }

                case 'q':
                {
                    unsigned int value = boost::any_cast<signed int>(*it2);
                    data.push_back(value);
                    break;
                }

                case 'l':
                {
                    unsigned long long value = boost::any_cast<unsigned long long>(*it2);
                    data.push_back(*reinterpret_cast<unsigned int *>(&value));
                    data.push_back(*reinterpret_cast<unsigned int *>((unsigned) &value + 4));
                    break;
                }

                case 'w':
                {
                    signed long long value = boost::any_cast<signed long long>(*it2);
                    data.push_back(*reinterpret_cast<unsigned int *>(&value));
                    data.push_back(*reinterpret_cast<unsigned int *>((unsigned) &value + 4));
                    break;
                }

                case 'f':
                {
                    double value = boost::any_cast<double>(*it2);
                    data.push_back(*reinterpret_cast<unsigned int *>(&value));
                    data.push_back(*reinterpret_cast<unsigned int *>((unsigned) &value + 4));
                    break;
                }

                case 'p':
                {
                    void *value = boost::any_cast<void *>(*it2);
                    data.push_back(reinterpret_cast<unsigned int>(value));
                    break;
                }

                case 's':
                {
                    const string *value = boost::any_cast<string>(&*it2);
                    data.push_back(reinterpret_cast<unsigned int>(value->c_str()));
                    break;
                }

                default:
                    throw runtime_error("C++ call: Unknown argument identifier " + *it);
            }
        }

        unsigned int result_low;
        unsigned int result_high;
        unsigned int *source = &data[0];
        unsigned int size = data.size() * 4;

        asm(
            "MOV EDI,ESP\n"
            "SUB EDI,%3\n"  // allocate memory in stack.
            "MOV ESI,%4\n"  // move ptr of source to ESI.
            "MOV ECX,%3\n"  // length of data.
            "PUSH DS\n"     // move DS
            "POP ES\n"      //         to ES.
            "CLD\n"         // clear direction flag.
            "REP MOVSB\n"   // Move bytes at address DS:ESI to address ES:EDI (move to stack).
            "MOV ESI,ESP\n" // stack pointer.
            "SUB ESP,%3\n"
            "CALL %2\n"
            "MOV ESP,ESI\n"
            "MOV %0,EAX\n"  // move low result from eax
            "MOV %1,EDX\n"  // move high result from edx
        : "=m"(result_low), "=m"(result_high)
        : "m"(fCpp) //2, "m"(size) //3, "m"(source) //4
        : "eax", "edx", "ecx", "esi", "edi", "cc"
        );

        *reinterpret_cast<unsigned int *>(&result) = result_low;
        *reinterpret_cast<unsigned int *>(((unsigned) &result) + 4) = result_high;
#else
        throw runtime_error("x64 Not supported yet (builtin timers and [Call/Make]Public");
#endif
#endif
    }

    return result;
}
