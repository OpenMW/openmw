//
// Created by koncord on 24.01.16.
//

#ifndef SCRIPTFUNCTIONS_HPP
#define SCRIPTFUNCTIONS_HPP

#include <Script/Functions/CharClass.hpp>
#include <Script/Functions/Positions.hpp>
#include <Script/Functions/Cells.hpp>
#include <Script/Functions/GUI.hpp>
#include <Script/Functions/Stats.hpp>
#include <Script/Functions/Items.hpp>
#include <Script/Functions/Quests.hpp>
#include <Script/Functions/Spells.hpp>
#include <Script/Functions/World.hpp>
#include <RakNetTypes.h>
//#include <amx/amx.h>
#include <tuple>
#include <apps/openmw-mp/Player.hpp>
#include "ScriptFunction.hpp"
#include "Types.hpp"

#define GET_PLAYER(pid, pl, retvalue) \
     pl = Players::getPlayer(pid); \
     if (player == 0) {\
        fprintf(stderr, "%s: Player with pid \'%d\' not found\n", __PRETTY_FUNCTION__, pid);\
        /*ScriptFunctions::StopServer(1);*/ \
        return retvalue;\
}


class ScriptFunctions
{
public:

    static void GetArguments(std::vector<boost::any> &params, va_list args, const std::string &def);

    static void StopServer(int code) noexcept;

    static void MakePublic(ScriptFunc _public, const char *name, char ret_type, const char *def) noexcept;
    static boost::any CallPublic(const char *name, ...) noexcept;

    static void SendMessage(unsigned short pid, const char *message, bool broadcast) noexcept;
    static void CleanChat(unsigned short pid);
    static void CleanChat();

    /**
     * \brief Create timer
     * \param callback
     * \param msec
     * \return return timer id
     */
    static int CreateTimer(ScriptFunc callback, int msec) noexcept;
    static int CreateTimerEx(ScriptFunc callback, int msec, const char *types, ...) noexcept;

    static void StartTimer(int timerId) noexcept;
    static void StopTimer(int timerId) noexcept;
    static void RestartTimer(int timerId, int msec) noexcept;
    static void FreeTimer(int timerId) noexcept;
    static bool IsTimerElapsed(int timerId) noexcept;

    static void Kick(unsigned short pid) noexcept;
    static const char *GetServerVersion() noexcept;
    static const char *GetProtocolVersion() noexcept;
    static int GetAvgPing(unsigned short pid) noexcept;
    static void SetModname(const char* name) noexcept;
    static void SetHostname(const char* name) noexcept;
    static void SetServerPassword(const char *passw) noexcept;

    static constexpr ScriptFunctionData functions[]{
            {"CreateTimer",         ScriptFunctions::CreateTimer},
            {"CreateTimerEx",       reinterpret_cast<Function<void>>(ScriptFunctions::CreateTimerEx)},
            {"MakePublic",          ScriptFunctions::MakePublic},
            {"CallPublic",          reinterpret_cast<Function<void>>(ScriptFunctions::CallPublic)},

            {"StartTimer",          ScriptFunctions::StartTimer},
            {"StopTimer",           ScriptFunctions::StopTimer},
            {"RestartTimer",        ScriptFunctions::RestartTimer},
            {"FreeTimer",           ScriptFunctions::FreeTimer},
            {"IsTimerElapsed",      ScriptFunctions::IsTimerElapsed},

            {"StopServer",          ScriptFunctions::StopServer},

            {"SendMessage",         ScriptFunctions::SendMessage},
            {"Kick",                ScriptFunctions::Kick},
            {"GetServerVersion",    ScriptFunctions::GetServerVersion},
            {"GetProtocolVersion",  ScriptFunctions::GetProtocolVersion},
            {"GetAvgPing",          ScriptFunctions::GetAvgPing},
            {"SetModname",          ScriptFunctions::SetModname},
            {"SetHostname",         ScriptFunctions::SetHostname},
            {"SetServerPassword",   ScriptFunctions::SetServerPassword},

            POSITIONAPI,
            CELLAPI,
            STATSFUNCTIONS,
            ITEMAPI,
            QUESTAPI,
            SPELLAPI,
            GUIFUNCTIONS,
            CHARCLASSFUNCTIONS,
            WORLDFUNCTIONS,
    };

    static constexpr ScriptCallbackData callbacks[]{
            {"Main",                     Function<int, int, int>()},
            {"OnServerInit",             Function<void>()},
            {"OnServerExit",             Function<void, bool>()},
            {"OnPlayerConnect",          Function<bool, unsigned short>()},
            {"OnPlayerDisconnect",       Function<void, unsigned short>()},
            {"OnPlayerDeath",            Function<void, unsigned short, short, unsigned short>()},
            {"OnPlayerResurrect",        Function<void, unsigned short>()},
            {"OnPlayerCellChange",       Function<void, unsigned short>()},
            {"OnPlayerCellState",        Function<void, unsigned short>()},
            {"OnPlayerAttributesChange", Function<void, unsigned short>()},
            {"OnPlayerSkillsChange",     Function<void, unsigned short>()},
            {"OnPlayerLevelChange",      Function<void, unsigned short>()},
            {"OnPlayerEquipmentChange",  Function<void, unsigned short>()},
            {"OnPlayerInventoryChange",  Function<void, unsigned short>()},
            {"OnPlayerSpellbookChange",  Function<void, unsigned short>()},
            {"OnPlayerJournalChange",    Function<void, unsigned short>()},
            {"OnObjectPlace",            Function<void, unsigned short, const char*>()},
            {"OnObjectDelete",           Function<void, unsigned short, const char*>()},
            {"OnObjectScale",            Function<void, unsigned short, const char*>()},
            {"OnObjectLock",             Function<void, unsigned short, const char*>()},
            {"OnObjectUnlock",           Function<void, unsigned short, const char*>()},
            {"OnDoorState",              Function<void, unsigned short, const char*>()},
            {"OnContainer",              Function<void, unsigned short, const char*>()},
            {"OnPlayerSendMessage",      Function<bool, unsigned short, const char*>()},
            {"OnPlayerEndCharGen",       Function<void, unsigned short>()},
            {"OnGUIAction",              Function<void, unsigned short, int, const char*>()}
    };
};

#endif //SCRIPTFUNCTIONS_HPP