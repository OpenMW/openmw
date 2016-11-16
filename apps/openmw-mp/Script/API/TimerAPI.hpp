//
// Created by koncord on 15.03.16.
//

#ifndef OPENMW_TIMERAPI_HPP
#define OPENMW_TIMERAPI_HPP

#include <string>

#include <Script/Script.hpp>
#include <Script/ScriptFunction.hpp>

namespace mwmp
{

    class TimerAPI;

    class Timer: public ScriptFunction
    {
        friend class TimerAPI;

    public:

        Timer(ScriptFunc callback, long msec, const std::string& def, std::vector<boost::any> args);
#if defined(ENABLE_PAWN)
        Timer(AMX *amx, ScriptFuncPAWN callback, long msec, const std::string& def, std::vector<boost::any> args);
#endif
#if defined(ENABLE_LUA)
        Timer(lua_State *lua, ScriptFuncLua callback, long msec, const std::string& def, std::vector<boost::any> args);
#endif
        void Tick();

        bool IsEnd();
        void Stop();
        void Start();
        void Restart(int msec);
    private:
        double startTime, targetMsec;
        std::string publ, arg_types;
        std::vector<boost::any> args;
        Script *scr;
        bool end;
    };

    class TimerAPI
    {
    public:
#if defined(ENABLE_PAWN)
        static int CreateTimerPAWN(AMX *amx, ScriptFuncPAWN callback, long msec, const std::string& def, std::vector<boost::any> args);
#endif
#if defined(ENABLE_LUA)
        static int CreateTimerLua(lua_State *lua, ScriptFuncLua callback, long msec, const std::string& def, std::vector<boost::any> args);
#endif
        static int CreateTimer(ScriptFunc callback, long msec, const std::string& def, std::vector<boost::any> args);
        static void FreeTimer(int timerid);
        static void ResetTimer(int timerid, long msec);
        static void StartTimer(int timerid);
        static void StopTimer(int timerid);
        static bool IsEndTimer(int timerid);

        static void Terminate();

        static void Tick();
    private:
        static std::unordered_map<int, Timer* > timers;
        static int pointer;
    };
}

#endif //OPENMW_TIMERAPI_HPP
