//
// Created by koncord on 24.01.16.
//

#ifndef SOURCEPAWN_SCRIPTFUNCTIONS_HPP
#define SOURCEPAWN_SCRIPTFUNCTIONS_HPP

#include <Script/Functions/CharClass.hpp>
#include <Script/Functions/Translocations.hpp>
#include <Script/Functions/GUI.hpp>
#include <Script/Functions/Stats.hpp>
#include <Script/Functions/Items.hpp>
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
        /*ScriptFunctions::stopServer(1);*/ \
        return retvalue;\
}


class ScriptFunctions
{
public:

    static void getArguments(std::vector<boost::any> &params, va_list args, const std::string &def);

    static void stopServer(int code) noexcept;

    static void makePublic(ScriptFunc _public, const char *name, char ret_type, const char *def) noexcept;
    static boost::any callPublic(const char *name, ...) noexcept;

    static void sendMessage(unsigned short pid, const char *message, bool broadcast) noexcept;
    static void cleanChat(unsigned short pid);
    static void cleanChat();

    /**
     * \brief Create timer
     * \param callback
     * \param msec
     * \return return timer id
     */
    static int createTimer(ScriptFunc callback, int msec) noexcept;
    static int createTimerEx(ScriptFunc callback, int msec, const char *types, ...) noexcept;

    static void startTimer(int timerId) noexcept;
    static void stopTimer(int timerId) noexcept;
    static void restartTimer(int timerId, int msec) noexcept;
    static void freeTimer(int timerId) noexcept;
    static bool isTimerElapsed(int timerId) noexcept;

    static void kick(unsigned short pid) noexcept;
    static const char *getServerVersion() noexcept;
    static const char *getProtocolVersion() noexcept;
    static int getAvgPing(unsigned short pid) noexcept;

    static constexpr ScriptFunctionData functions[]{
            {"createTimer",         ScriptFunctions::createTimer},
            {"createTimerEx",       reinterpret_cast<Function<void>>(ScriptFunctions::createTimerEx)},
            {"makePublic",          ScriptFunctions::makePublic},
            {"callPublic",          reinterpret_cast<Function<void>>(ScriptFunctions::callPublic)},


            {"startTimer",          ScriptFunctions::startTimer},
            {"stopTimer",           ScriptFunctions::stopTimer},
            {"restartTimer",        ScriptFunctions::restartTimer},
            {"freeTimer",           ScriptFunctions::freeTimer},
            {"isTimerElapsed",      ScriptFunctions::isTimerElapsed},

            {"stopServer",          ScriptFunctions::stopServer},

            {"sendMessage",         ScriptFunctions::sendMessage},
            {"kick",                ScriptFunctions::kick},
            {"getServerVersion",    ScriptFunctions::getServerVersion},
            {"getProtocolVersion",  ScriptFunctions::getProtocolVersion},
            {"getAvgPing",          ScriptFunctions::getAvgPing},

            TRANSLOCATIONFUNCTIONS,
            STATSFUNCTIONS,
            ITEMAPI,
            GUIFUNCTIONS,
            CHARCLASSFUNCTIONS,
            WORLDFUNCTIONS,


    };

    static constexpr ScriptCallbackData callbacks[]{
            {"Main",                     Function<int, int, int>()},
            {"onServerInit",             Function<void>()},
            {"onServerExit",             Function<void, bool>()},
            {"onPlayerConnect",          Function<bool, unsigned short>()},
            {"onPlayerDisconnect",       Function<void, unsigned short>()},
            {"onPlayerDeath",            Function<void, unsigned short, short, unsigned short>()},
            {"onPlayerResurrect",        Function<void, unsigned short>()},
            {"onPlayerChangeCell",       Function<void, unsigned short>()},
            {"onPlayerChangeAttributes", Function<void, unsigned short>()},
            {"onPlayerChangeSkills",     Function<void, unsigned short>()},
            {"onPlayerChangeLevel",      Function<void, unsigned short>()},
            {"onPlayerChangeEquipment",  Function<void, unsigned short>()},
            {"onPlayerChangeInventory",  Function<void, unsigned short>()},
            {"onPlayerSendMessage",      Function<bool, unsigned short, const char*>()},
            {"onPlayerEndCharGen",       Function<void, unsigned short>()},
            {"onGuiAction",              Function<void, unsigned short, int, const char*>()}
    };
};

#endif //SOURCEPAWN_SCRIPTFUNCTIONS_HPP