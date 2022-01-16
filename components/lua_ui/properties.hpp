#ifndef OPENMW_LUAUI_PROPERTIES
#define OPENMW_LUAUI_PROPERTIES

#include <sol/sol.hpp>
#include <MyGUI_Types.h>
#include <osg/Vec2>

#include <components/lua/luastate.hpp>

namespace LuaUi
{
    template <typename T>
    sol::optional<T> getProperty(sol::object from, std::string_view field) {
        sol::object value = LuaUtil::getFieldOrNil(from, field);
        if (value == sol::nil)
            return sol::nullopt;
        if (value.is<T>())
            return value.as<T>();
        std::string error("Property \"");
        error += field;
        error += "\" has an invalid value \"";
        error += LuaUtil::toString(value);
        error += "\"";
        throw std::logic_error(error);
    }

    template<typename T>
    T parseProperty(sol::object from, std::string_view field, const T& defaultValue)
    {
        sol::optional<T> opt = getProperty<T>(from, field);
        if (opt.has_value())
            return opt.value();
        else
            return defaultValue;
    }

    template <typename T>
    MyGUI::types::TPoint<T> parseProperty(
        sol::object from,
        std::string_view field,
        const MyGUI::types::TPoint<T>& defaultValue)
    {
        auto v = getProperty<osg::Vec2f>(from, field);
        if (v.has_value())
            return MyGUI::types::TPoint<T>(v.value().x(), v.value().y());
        else
            return defaultValue;
    }

    template <typename T>
    MyGUI::types::TSize<T> parseProperty(
        sol::object from,
        std::string_view field,
        const MyGUI::types::TSize<T>& defaultValue)
    {
        auto v = getProperty<osg::Vec2f>(from, field);
        if (v.has_value())
            return MyGUI::types::TSize<T>(v.value().x(), v.value().y());
        else
            return defaultValue;
    }
}

#endif // !OPENMW_LUAUI_PROPERTIES
