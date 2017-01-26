//
// Created by koncord on 09.05.16.
//

#include <iostream>
#include "LangLua.hpp"
#include <Script/API/TimerAPI.hpp>
#include <Script/API/PublicFnAPI.hpp>

using namespace std;

inline vector<boost::any> DefToVec(lua_State *lua, string types, int args_begin, int args_n)
{
    vector<boost::any> args;

    for (int i = args_begin; i < args_n + args_begin; i++)
    {
        switch (types[i - args_begin])
        {
            case 'i':
            {
                args.emplace_back(luabridge::Stack<unsigned int>::get(lua, i));
                break;
            }

            case 'q':
            {
                args.emplace_back(luabridge::Stack<signed int>::get(lua, i));
                break;
            }

                /*case 'l':
                {
                    args.emplace_back(luabridge::Stack<unsigned long long>::get(lua, i));
                    break;
                }

                case 'w':
                {
                    args.emplace_back(luabridge::Stack<signed long long>::get(lua, i));
                    break;
                }*/

            case 'f':
            {
                args.emplace_back(luabridge::Stack<double>::get(lua, i));
                break;
            }

            case 's':
            {
                args.emplace_back(luabridge::Stack<const char*>::get(lua, i));
                break;
            }

            default:
            {
                stringstream ssErr;
                ssErr << "Lua: Unknown argument identifier" << "\"" << types[i] << "\"" << endl;
                throw std::runtime_error(ssErr.str());
            }
        }
    }
    return args;
}

int LangLua::MakePublic(lua_State *lua) noexcept
{
    const char * callback = luabridge::Stack<const char*>::get(lua, 1);
    const char * name = luabridge::Stack<const char*>::get(lua, 2);
    char ret_type = luabridge::Stack<char>::get(lua, 3);
    const char * def = luabridge::Stack<const char*>::get(lua, 4);

    Public::MakePublic(callback, lua, name, ret_type, def);
    return 0;

}

int LangLua::CallPublic(lua_State *lua)
{
    const char * name = luabridge::Stack<const char*>::get(lua, 1);

    int args_n = lua_gettop(lua) - 1;

    string types = Public::GetDefinition(name);

    if (args_n  != (long)types.size())
        throw invalid_argument("Script call: Number of arguments does not match definition");

    vector<boost::any> args = DefToVec(lua, types, 2, args_n);

    boost::any result = Public::Call(&name[0], args);
    if (result.empty())
        return 0;

    if (result.type().hash_code() == typeid(signed int).hash_code())
        luabridge::Stack<signed int>::push(lua, boost::any_cast<signed int>(result));
    else if (result.type().hash_code() == typeid(unsigned int).hash_code())
        luabridge::Stack<unsigned int>::push(lua, boost::any_cast<unsigned int>(result));
    else if (result.type().hash_code() == typeid(double).hash_code())
        luabridge::Stack<double>::push(lua, boost::any_cast<double>(result));
    else if (result.type().hash_code() == typeid(const char*).hash_code())
        luabridge::Stack<const char*>::push(lua, boost::any_cast<const char*>(result));
    return 1;
}

int LangLua::CreateTimer(lua_State *lua) noexcept
{

    const char * callback= luabridge::Stack<const char*>::get(lua, 1);
    int msec = luabridge::Stack<int>::get(lua, 2);

    int id = mwmp::TimerAPI::CreateTimerLua(lua, callback, msec, "", vector<boost::any>());
    luabridge::push(lua, id);
    return 1;
}

int LangLua::CreateTimerEx(lua_State *lua)
{
    const char * callback = luabridge::Stack<const char*>::get(lua, 1);
    int msec = luabridge::Stack<int>::get(lua, 2);

    const char * types = luabridge::Stack<const char*>::get(lua, 3);

    int args_n = (int)lua_strlen(lua, 3);

    vector<boost::any> args;

    for (int i = 4; i < args_n + 4; i++)
    {
        switch (types[i - 4])
        {
            case 'i':
            {
                args.emplace_back(luabridge::Stack<unsigned int>::get(lua, i));
                break;
            }

            case 'q':
            {
                args.emplace_back(luabridge::Stack<signed int>::get(lua, i));
                break;
            }

                /*case 'l':
                {
                    args.emplace_back(luabridge::Stack<unsigned long long>::get(lua, i));
                    break;
                }

                case 'w':
                {
                    args.emplace_back(luabridge::Stack<signed long long>::get(lua, i));
                    break;
                }*/

            case 'f':
            {
                args.emplace_back(luabridge::Stack<double>::get(lua, i));
                break;
            }

            case 's':
            {
                args.emplace_back(luabridge::Stack<const char*>::get(lua, i));
                break;
            }

            default:
            {
                stringstream ssErr;
                ssErr << "Lua: Unknown argument identifier" << "\"" << types[i] << "\"" << endl;
                throw std::runtime_error(ssErr.str());
            }
        }
    }


    int id = mwmp::TimerAPI::CreateTimerLua(lua, callback, msec, types, args);
    luabridge::push(lua, id);
    return 1;
}
