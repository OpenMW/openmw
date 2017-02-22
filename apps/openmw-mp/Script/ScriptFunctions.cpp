//
// Created by koncord on 24.01.16.
//

#include "ScriptFunctions.hpp"
#include "API/PublicFnAPI.hpp"
#include <cstdarg>
#include <iostream>
#include <apps/openmw-mp/Player.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Version.hpp>
#include "MasterClient.hpp"

template<typename... Types>
constexpr char TypeString<Types...>::value[];
constexpr ScriptFunctionData ScriptFunctions::functions[];
constexpr ScriptCallbackData ScriptFunctions::callbacks[];

using namespace std;

void ScriptFunctions::GetArguments(std::vector<boost::any> &params, va_list args, const std::string &def)
{
    params.reserve(def.length());

    try
    {
        for (char c : def)
        {
            switch (c)
            {
                case 'i':
                    params.emplace_back(va_arg(args, unsigned int));
                    break;

                case 'q':
                    params.emplace_back(va_arg(args, signed int));
                    break;

                case 'l':
                    params.emplace_back(va_arg(args, unsigned long long));
                    break;

                case 'w':
                    params.emplace_back(va_arg(args, signed long long));
                    break;

                case 'f':
                    params.emplace_back(va_arg(args, double));
                    break;

                case 'p':
                    params.emplace_back(va_arg(args, void*));
                    break;

                case 's':
                    params.emplace_back(va_arg(args, const char*));
                    break;

                case 'b':
                    params.emplace_back(va_arg(args, int));
                    break;

                default:
                    throw runtime_error("C++ call: Unknown argument identifier " + c);
            }
        }
    }

    catch (...)
    {
        va_end(args);
        throw;
    }
    va_end(args);
}

void ScriptFunctions::MakePublic(ScriptFunc _public, const char *name, char ret_type, const char *def) noexcept
{
    Public::MakePublic(_public, name, ret_type, def);
}

boost::any ScriptFunctions::CallPublic(const char *name, ...) noexcept
{
    vector<boost::any> params;

    try
    {
        string def = Public::GetDefinition(name);

        va_list args;
        va_start(args, name);
        GetArguments(params, args, def);
        va_end(args);

        return Public::Call(name, params);
    }
    catch (...) {}

    return 0;
}

void ScriptFunctions::StopServer(int code) noexcept
{
    mwmp::Networking::getPtr()->stopServer(code);
}

void ScriptFunctions::Kick(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);
    mwmp::Networking::getPtr()->kickPlayer(player->guid);
}

const char *ScriptFunctions::GetServerVersion() noexcept
{
    return TES3MP_VERSION;
}

const char *ScriptFunctions::GetProtocolVersion() noexcept
{
    static string version = to_string(TES3MP_PROTO_VERSION);
    return version.c_str();
}

int ScriptFunctions::GetAvgPing(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,-1);
    return mwmp::Networking::get().getAvgPing(player->guid);
}

void ScriptFunctions::SetModname(const char *name) noexcept
{
    mwmp::Networking::getPtr()->getMasterClient()->SetModname(name);
}

void ScriptFunctions::SetHostname(const char *name) noexcept
{
    mwmp::Networking::getPtr()->getMasterClient()->SetHostname(name);
}

void ScriptFunctions::SetServerPassword(const char *passw) noexcept
{
    mwmp::Networking::getPtr()->setServerPassword(passw);
}
