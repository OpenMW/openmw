//
// Created by koncord on 15.03.16.
//

#include "TimerAPI.hpp"

#include <chrono>

#include <iostream>
using namespace mwmp;
using namespace std;

Timer::Timer(ScriptFunc callback, long msec, const std::string& def, std::vector<boost::any> args) : ScriptFunction(callback, 'v', def)
{
    targetMsec = msec;
    this->args = args;
    end = true;
}

#if defined(ENABLE_PAWN)
Timer::Timer(AMX *amx, ScriptFuncPAWN callback, long msec, const std::string &def, std::vector<boost::any> args): ScriptFunction(callback, amx, 'v', def)
{
    targetMsec = msec;
    this->args = args;
    end = true;
}
#endif
#if defined(ENABLE_LUA)
Timer::Timer(lua_State *lua, ScriptFuncLua callback, long msec, const std::string& def, std::vector<boost::any> args): ScriptFunction(callback, lua, 'v', def)
{
    targetMsec = msec;
    this->args = args;
    end = true;
}
#endif

void Timer::Tick()
{
    if (end)
        return;

    const auto duration = chrono::system_clock::now().time_since_epoch();
    const auto time = chrono::duration_cast<chrono::milliseconds>(duration).count();

    if (time - startTime >= targetMsec)
    {
        Call(args);
        end = true;
    }
}

bool Timer::IsEnd()
{
    return end;
}

void Timer::Stop()
{
    end = true;
}

void Timer::Restart(int msec)
{
    targetMsec = msec;
    Start();
}

void Timer::Start()
{
    end = false;

    const auto duration = chrono::system_clock::now().time_since_epoch();
    const auto msec = chrono::duration_cast<chrono::milliseconds>(duration).count();
    startTime = msec;
}

int TimerAPI::pointer = 0;
std::unordered_map<int, Timer* > TimerAPI::timers;

#if defined(ENABLE_PAWN)
int TimerAPI::CreateTimerPAWN(AMX *amx, ScriptFuncPAWN callback, long msec, const string& def, std::vector<boost::any> args)
{
    int id = -1;

    for (auto timer : timers)
    {
        if (timer.second != nullptr)
            continue;
        timer.second = new Timer(amx, callback, msec, def, args);
        id = timer.first;
    }

    if (id == -1)
    {
        timers[pointer] = new Timer(amx, callback, msec, def, args);
        id = pointer;
        pointer++;
    }

    return id;
}
#endif

#if defined(ENABLE_LUA)
int TimerAPI::CreateTimerLua(lua_State *lua, ScriptFuncLua callback, long msec, const std::string& def, std::vector<boost::any> args)
{
    int id = -1;

    for (auto timer : timers)
    {
        if (timer.second != nullptr)
            continue;
        timer.second = new Timer(lua, callback, msec, def, args);
        id = timer.first;
    }

    if (id == -1)
    {
        timers[pointer] = new Timer(lua, callback, msec, def, args);
        id = pointer;
        pointer++;
    }

    return id;
}
#endif


int TimerAPI::CreateTimer(ScriptFunc callback, long msec, const std::string &def, std::vector<boost::any> args)
{
    int id = -1;

    for (auto timer : timers)
    {
        if (timer.second != nullptr)
            continue;
        timer.second = new Timer(callback, msec, def, args);
        id = timer.first;
    }

    if (id == -1)
    {
        timers[pointer] = new Timer(callback, msec, def, args);
        id = pointer;
        pointer++;
    }

    return id;
}

void TimerAPI::FreeTimer(int timerid)
{

    try
    {
        if (timers.at(timerid) != nullptr)
        {
            delete timers[timerid];
            timers[timerid] = nullptr;
        }
    }
    catch(...)
    {
        std::cerr << "Timer " << timerid << " not found!" << endl;
    }
}

void TimerAPI::ResetTimer(int timerid, long msec)
{
    try
    {
        timers.at(timerid)->Restart(msec);
    }
    catch(...)
    {
        std::cerr << "Timer " << timerid << " not found!" << endl;
    }
}

void TimerAPI::StartTimer(int timerid)
{
    try
    {
        Timer *timer = timers.at(timerid);
        if (timer == nullptr)
            throw 1;
        timer->Start();
    }
    catch(...)
    {
        std::cerr << "Timer " << timerid << " not found!" << endl;
    }
}

void TimerAPI::StopTimer(int timerid)
{
    try
    {
        timers.at(timerid)->Stop();
    }
    catch(...)
    {
        std::cerr << "Timer " << timerid << " not found!" << endl;
    }
}

bool TimerAPI::IsEndTimer(int timerid)
{
    bool ret = false;
    try
    {
        ret = timers.at(timerid)->IsEnd();
    }
    catch(...)
    {
        std::cerr << "Timer " << timerid << " not found!" << endl;
    }
    return ret;
}

void TimerAPI::Terminate()
{
    for (auto timer : timers)
    {
        if (timer.second != nullptr)
            delete timer.second;
        timer.second = nullptr;
    }
}

void TimerAPI::Tick()
{
    for (auto timer : timers)
    {
        if (timer.second != nullptr)
            timer.second->Tick();
    }
}
