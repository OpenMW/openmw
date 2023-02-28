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

    Callback Callback::fromLua(const sol::table& t)
    {
        return Callback{ t.raw_get<sol::main_protected_function>(1), t.raw_get<AsyncPackageId>(2).mHiddenData };
    }

    bool Callback::isLuaCallback(const sol::object& t)
    {
        if (!t.is<sol::table>())
            return false;
        sol::object meta = sol::table(t)[sol::metatable_key];
        if (!meta.is<sol::table>())
            return false;
        return sol::table(meta).raw_get_or<bool, std::string_view, bool>("isCallback", false);
    }

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

        sol::table callbackMeta = sol::table::create(L);
        callbackMeta[sol::meta_function::call] = [](const sol::table& callback, sol::variadic_args va) {
            return Callback::fromLua(callback).call(sol::as_args(va));
        };
        callbackMeta[sol::meta_function::to_string] = [] { return "Callback"; };
        callbackMeta[sol::meta_function::metatable] = false;
        callbackMeta["isCallback"] = true;
        api["callback"] = [callbackMeta](const AsyncPackageId& asyncId, sol::main_protected_function fn) -> sol::table {
            sol::table c = sol::table::create(fn.lua_state(), 2);
            c.raw_set(1, std::move(fn), 2, asyncId);
            c[sol::metatable_key] = callbackMeta;
            return c;
        };

        auto initializer = [](sol::table hiddenData) {
            ScriptsContainer::ScriptId id = hiddenData[ScriptsContainer::sScriptIdKey];
            return AsyncPackageId{ id.mContainer, id.mIndex, hiddenData };
        };
        return sol::make_object(lua, initializer);
    }

}
