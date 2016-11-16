//
// Created by koncord on 15.03.16.
//

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <Player.hpp>
#include <Networking.hpp>
#include <Script/API/TimerAPI.hpp>

using namespace std;
using namespace mwmp;

int ScriptFunctions::createTimer(ScriptFunc callback, int msec) noexcept
{
    return -1;
}

int ScriptFunctions::createTimerEx(ScriptFunc callback, int msec, const char *types, ...) noexcept
{
    return -1;
}

void ScriptFunctions::startTimer(int timerId) noexcept
{
    TimerAPI::startTimer(timerId);
}

void ScriptFunctions::stopTimer(int timerId) noexcept
{
    TimerAPI::stopTimer(timerId);
}

void ScriptFunctions::restartTimer(int timerId, int msec) noexcept
{
    TimerAPI::resetTimer(timerId, msec);
}

void ScriptFunctions::freeTimer(int timerId) noexcept
{
    TimerAPI::freeTimer(timerId);
}

bool ScriptFunctions::isTimerElapsed(int timerId) noexcept
{
    return TimerAPI::isEndTimer(timerId);
}


