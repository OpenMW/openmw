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

int ScriptFunctions::CreateTimer(ScriptFunc callback, int msec) noexcept
{
    return -1;
}

int ScriptFunctions::CreateTimerEx(ScriptFunc callback, int msec, const char *types, ...) noexcept
{
    return -1;
}

void ScriptFunctions::StartTimer(int timerId) noexcept
{
    TimerAPI::StartTimer(timerId);
}

void ScriptFunctions::StopTimer(int timerId) noexcept
{
    TimerAPI::StopTimer(timerId);
}

void ScriptFunctions::RestartTimer(int timerId, int msec) noexcept
{
    TimerAPI::ResetTimer(timerId, msec);
}

void ScriptFunctions::FreeTimer(int timerId) noexcept
{
    TimerAPI::FreeTimer(timerId);
}

bool ScriptFunctions::IsTimerElapsed(int timerId) noexcept
{
    return TimerAPI::IsEndTimer(timerId);
}
