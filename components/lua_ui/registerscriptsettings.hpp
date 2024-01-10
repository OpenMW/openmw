#ifndef OPENMW_LUAUI_REGISTERSCRIPTSETTINGS
#define OPENMW_LUAUI_REGISTERSCRIPTSETTINGS

#include <sol/sol.hpp>

namespace LuaUi
{
    // implemented in scriptsettings.cpp
    void registerSettingsPage(const sol::table& options);
    void clearSettings();
    void removeSettingsPage(std::string_view key);
}

#endif // !OPENMW_LUAUI_REGISTERSCRIPTSETTINGS
