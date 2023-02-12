#include "asyncpackage.hpp"

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::AsyncPackageId> : std::false_type
    {
    };

    template <>
    struct is_automagical<LuaUtil::Callback> : std::false_type
    {
    };
}

namespace LuaUtil
{

    struct TimerCallback
    {
        AsyncPackageId mAsyncId;
        std::string mName;
    };

    sol::function getAsyncPackageInitializer(
        lua_State* L, std::function<double()> simulationTimeFn, std::function<double()> gameTimeFn)
    {
        sol::state_view lua(L);
        using TimerType = ScriptsContainer::TimerType;
        sol::usertype<AsyncPackageId> api = lua.new_usertype<AsyncPackageId>("AsyncPackage");
        api["registerTimerCallback"]
            = [](const AsyncPackageId& asyncId, std::string_view name, sol::main_protected_function callback) {
                  asyncId.mContainer->registerTimerCallback(asyncId.mScriptId, name, std::move(callback));
                  return TimerCallback{ asyncId, std::string(name) };
              };
        api["newSimulationTimer"] = [simulationTimeFn](const AsyncPackageId&, double delay,
                                        const TimerCallback& callback, sol::main_object callbackArg) {
            callback.mAsyncId.mContainer->setupSerializableTimer(TimerType::SIMULATION_TIME, simulationTimeFn() + delay,
                callback.mAsyncId.mScriptId, callback.mName, std::move(callbackArg));
        };
        api["newGameTimer"] = [gameTimeFn](const AsyncPackageId&, double delay, const TimerCallback& callback,
                                  sol::main_object callbackArg) {
            callback.mAsyncId.mContainer->setupSerializableTimer(TimerType::GAME_TIME, gameTimeFn() + delay,
                callback.mAsyncId.mScriptId, callback.mName, std::move(callbackArg));
        };
        api["newUnsavableSimulationTimer"]
            = [simulationTimeFn](const AsyncPackageId& asyncId, double delay, sol::main_protected_function callback) {
                  asyncId.mContainer->setupUnsavableTimer(
                      TimerType::SIMULATION_TIME, simulationTimeFn() + delay, asyncId.mScriptId, std::move(callback));
              };
        api["newUnsavableGameTimer"]
            = [gameTimeFn](const AsyncPackageId& asyncId, double delay, sol::main_protected_function callback) {
                  asyncId.mContainer->setupUnsavableTimer(
                      TimerType::GAME_TIME, gameTimeFn() + delay, asyncId.mScriptId, std::move(callback));
              };
        api["callback"] = [](const AsyncPackageId& asyncId, sol::main_protected_function fn) -> Callback {
            return Callback{ std::move(fn), asyncId.mHiddenData };
        };

        sol::usertype<Callback> callbackType = lua.new_usertype<Callback>("Callback");
        callbackType[sol::meta_function::call]
            = [](const Callback& callback, sol::variadic_args va) { return callback.call(sol::as_args(va)); };

        auto initializer = [](sol::table hiddenData) {
            ScriptId id = hiddenData[ScriptsContainer::sScriptIdKey];
            return AsyncPackageId{ id.mContainer, id.mIndex, hiddenData };
        };
        return sol::make_object(lua, initializer);
    }

}
