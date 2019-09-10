#include "util.hpp"

#include <components/debug/debuglog.hpp>

namespace MWLua
{
    void logStackTrace(const char* message)
    {
        if (message != nullptr) {
            Log(Debug::Error) << message;
        }

        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;
        sol::protected_function_result result = state["debug"]["traceback"]();
        if (result.valid())
        {
            sol::optional<std::string> asString = result;
            if (asString)
            {
                Log(Debug::Error) << asString.value();
            }
        }
    }

    sol::object makeLuaObject(MWWorld::Ptr object)
    {
        LuaManager& luaManager = LuaManager::getInstance();
        auto stateHandle = luaManager.getThreadSafeStateHandle();

        /*
        // Search in cache first.
        sol::object result = stateHandle.getCachedUserdata(object);
        if (result != sol::nil) {
            return result;
        }
        */

        sol::state& state = stateHandle.state;

        sol::object result = sol::make_object(state, object);

        /*
        // Insert the object into cache.
        if (result != sol::nil) {
            stateHandle.insertUserdataIntoCache(object, result);
        }
        */

        return result;
    }

    MWWorld::Ptr getOptionalParamReference(sol::optional<sol::table> maybeParams, const char* key)
    {
        MWWorld::Ptr value = MWWorld::Ptr();

        if (maybeParams)
        {
            sol::table params = maybeParams.value();
            sol::object maybeValue = params[key];
            if (maybeValue.valid())
            {
                //if (maybeValue.is<std::string>())
                //{
                //    value = tes3::getReference(maybeValue.as<std::string>());
                //}
                //else
                if (maybeValue.is<MWWorld::Ptr>())
                {
                    value = maybeValue.as<MWWorld::Ptr>();
                }
            }
        }

        return value;
    }

    MWWorld::Ptr getOptionalParamExecutionReference(sol::optional<sol::table> maybeParams)
    {
        MWWorld::Ptr value = MWWorld::Ptr();

        if (maybeParams)
        {
            sol::table params = maybeParams.value();
            sol::object maybeValue = params["reference"];
            if (maybeValue.valid())
            {
                //if (maybeValue.is<std::string>()) {
                //    value = tes3::getReference(maybeValue.as<std::string>());
                //}
                //else
                if (maybeValue.is<MWWorld::Ptr>())
                {
                    value = maybeValue.as<MWWorld::Ptr>();
                }
            }
        }

        if (value.isEmpty())
            value = LuaManager::getInstance().getCurrentReference();

        return value;
    }

    sol::optional<osg::Vec3f> getOptionalParamVector3(sol::optional<sol::table> maybeParams, const char* key)
    {
        if (maybeParams)
        {
            sol::table params = maybeParams.value();
            sol::object maybeValue = params[key];
            if (maybeValue.valid())
            {
                // Were we given a real vector?
                if (maybeValue.is<osg::Vec3f>())
                {
                    return maybeValue.as<osg::Vec3f>();
                }

                // Were we given a table?
                else if (maybeValue.get_type() == sol::type::table)
                {
                    sol::table value = maybeValue.as<sol::table>();
                    return osg::Vec3f(value[1], value[2], value[3]);
                }
            }
        }

        return sol::optional<osg::Vec3f>();
    }
}
