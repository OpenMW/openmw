#ifndef MWLUA_CONTEXT_H
#define MWLUA_CONTEXT_H

#include <components/lua/luastate.hpp>

namespace LuaUtil
{
    class LuaState;
    class UserdataSerializer;
}

namespace MWLua
{
    class LuaEvents;
    class LuaManager;
    class ObjectLists;

    struct Context
    {
        enum Type
        {
            Menu,
            Global,
            Local,
        };
        Type mType;
        LuaManager* mLuaManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        ObjectLists* mObjectLists;
        LuaEvents* mLuaEvents;

        std::string_view typeName() const
        {
            switch (mType)
            {
                case Menu:
                    return "menu";
                case Global:
                    return "global";
                case Local:
                    return "local";
                default:
                    throw std::domain_error("Unhandled context type");
            }
        }

        template <class... Str>
        sol::object getCachedPackage(std::string_view first, const Str&... str) const
        {
            sol::object package = sol()[first];
            if constexpr (sizeof...(str) == 0)
                return package;
            else
                return LuaUtil::getFieldOrNil(package, str...);
        }

        template <class... Str>
        const sol::object& setCachedPackage(const sol::object& value, std::string_view first, const Str&... str) const
        {
            sol::state_view lua = sol();
            if constexpr (sizeof...(str) == 0)
                lua[first] = value;
            else
            {
                if (lua[first] == sol::nil)
                    lua[first] = sol::table(lua, sol::create);
                sol::table table = lua[first];
                LuaUtil::setDeepField(table, value, str...);
            }
            return value;
        }

        sol::object getTypePackage(std::string_view key) const { return getCachedPackage(key, typeName()); }

        const sol::object& setTypePackage(const sol::object& value, std::string_view key) const
        {
            return setCachedPackage(value, key, typeName());
        }

        template <class Factory>
        sol::object cachePackage(std::string_view key, Factory factory) const
        {
            sol::object cached = getCachedPackage(key);
            if (cached != sol::nil)
                return cached;
            else
                return setCachedPackage(factory(), key);
        }

        bool initializeOnce(std::string_view key) const
        {
            auto view = sol();
            sol::object flag = view[key];
            view[key] = sol::make_object(view, true);
            return flag == sol::nil;
        }

        sol::state_view sol() const
        {
            // Bindings are initialized in a safe context
            return mLua->unsafeState();
        }
    };

}

#endif // MWLUA_CONTEXT_H
