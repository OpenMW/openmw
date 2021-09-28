#include "luabindings.hpp"

#include "luamanagerimp.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWLua::AsyncPackageId> : std::false_type {};
}

namespace MWLua
{

    struct TimerCallback
    {
        AsyncPackageId mAsyncId;
        std::string mName;
    };

    sol::function getAsyncPackageInitializer(const Context& context)
    {
        using TimeUnit = LuaUtil::ScriptsContainer::TimeUnit;
        sol::usertype<AsyncPackageId> api = context.mLua->sol().new_usertype<AsyncPackageId>("AsyncPackage");
        api["registerTimerCallback"] = [](const AsyncPackageId& asyncId, std::string_view name, sol::function callback)
        {
            asyncId.mContainer->registerTimerCallback(asyncId.mScript, name, std::move(callback));
            return TimerCallback{asyncId, std::string(name)};
        };
        api["newTimerInSeconds"] = [world=context.mWorldView](const AsyncPackageId&, double delay,
                                                              const TimerCallback& callback, sol::object callbackArg)
        {
            callback.mAsyncId.mContainer->setupSerializableTimer(
                TimeUnit::SECONDS, world->getGameTimeInSeconds() + delay,
                callback.mAsyncId.mScript, callback.mName, std::move(callbackArg));
        };
        api["newTimerInHours"] = [world=context.mWorldView](const AsyncPackageId&, double delay,
                                                            const TimerCallback& callback, sol::object callbackArg)
        {
            callback.mAsyncId.mContainer->setupSerializableTimer(
                TimeUnit::HOURS, world->getGameTimeInHours() + delay,
                callback.mAsyncId.mScript, callback.mName, std::move(callbackArg));
        };
        api["newUnsavableTimerInSeconds"] = [world=context.mWorldView](const AsyncPackageId& asyncId, double delay, sol::function callback)
        {
            asyncId.mContainer->setupUnsavableTimer(
                TimeUnit::SECONDS, world->getGameTimeInSeconds() + delay, asyncId.mScript, std::move(callback));
        };
        api["newUnsavableTimerInHours"] = [world=context.mWorldView](const AsyncPackageId& asyncId, double delay, sol::function callback)
        {
            asyncId.mContainer->setupUnsavableTimer(
                TimeUnit::HOURS, world->getGameTimeInHours() + delay, asyncId.mScript, std::move(callback));
        };
        api["callback"] = [](const AsyncPackageId& asyncId, sol::function fn)
        {
            return Callback{std::move(fn), asyncId.mHiddenData};
        };

        auto initializer = [](sol::table hiddenData)
        {
            LuaUtil::ScriptsContainer::ScriptId id = hiddenData[LuaUtil::ScriptsContainer::ScriptId::KEY];
            hiddenData[Callback::SCRIPT_NAME_KEY] = id.toString();
            return AsyncPackageId{id.mContainer, id.mPath, hiddenData};
        };
        return sol::make_object(context.mLua->sol(), initializer);
    }

}
