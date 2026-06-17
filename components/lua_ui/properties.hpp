#ifndef OPENMW_LUAUI_PROPERTIES
#define OPENMW_LUAUI_PROPERTIES

#include <MyGUI_Colour.h>
#include <MyGUI_Types.h>
#include <osg/Vec2>
#include <sol/sol.hpp>

#include <components/lua/luastate.hpp>
#include <components/misc/color.hpp>

namespace LuaUi
{
    template <typename T>
    constexpr bool isMyGuiIntVector()
    {
        return std::is_same<T, MyGUI::IntPoint>() || std::is_same<T, MyGUI::IntSize>();
    }

    template <typename T>
    constexpr bool isMyGuiVector()
    {
        return isMyGuiIntVector<T>() || std::is_same<T, MyGUI::FloatPoint>() || std::is_same<T, MyGUI::FloatSize>();
    }

    template <typename T>
    constexpr bool isMyGuiColor()
    {
        return std::is_same<T, MyGUI::Colour>();
    }

    template <typename T, typename LuaT>
    sol::optional<T> parseValue(const sol::object& table, std::string_view field, std::string_view errorPrefix)
    {
        sol::object opt = LuaUtil::getFieldOrNil(table, field);
        if (opt != sol::nil && !opt.is<LuaT>())
        {
            std::string error(errorPrefix);
            error += " \"";
            error += field;
            error += "\" has an invalid value \"";
            error += LuaUtil::toString(opt);
            error += "\"";
            throw std::logic_error(error);
        }
        if (!opt.is<LuaT>())
            return sol::nullopt;

        LuaT luaT = opt.as<LuaT>();
        if constexpr (isMyGuiIntVector<T>())
            return T(static_cast<int>(luaT.x()), static_cast<int>(luaT.y()));
        else if constexpr (isMyGuiVector<T>())
            return T(luaT.x(), luaT.y());
        else if constexpr (isMyGuiColor<T>())
            return T(luaT.r(), luaT.g(), luaT.b(), luaT.a());
        else
            return luaT;
    }

    template <typename T>
    sol::optional<T> parseValue(const sol::object& table, std::string_view field, std::string_view errorPrefix)
    {
        if constexpr (isMyGuiVector<T>())
            return parseValue<T, osg::Vec2f>(table, field, errorPrefix);
        else if constexpr (isMyGuiColor<T>())
            return parseValue<T, Misc::Color>(table, field, errorPrefix);
        else
            return parseValue<T, T>(table, field, errorPrefix);
    }

    template <typename T>
    T parseProperty(sol::object props, sol::object templateProps, std::string_view field, const T& defaultValue)
    {
        auto propOptional = parseValue<T>(props, field, "Property");
        auto templateOptional = parseValue<T>(templateProps, field, "Template property");

        if (propOptional.has_value())
            return propOptional.value();
        else if (templateOptional.has_value())
            return templateOptional.value();
        else
            return defaultValue;
    }

    template <typename T>
    T parseExternal(sol::object external, std::string_view field, const T& defaultValue)
    {
        auto optional = parseValue<T>(external, field, "External value");

        return optional.value_or(defaultValue);
    }
}

#endif // !OPENMW_LUAUI_PROPERTIES
