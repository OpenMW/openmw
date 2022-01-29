#ifndef OPENMW_LUAUI_SCRIPTSETTINGS
#define OPENMW_LUAUI_SCRIPTSETTINGS

#include <string>
#include <string_view>

#include <MyGUI_Widget.h>

namespace LuaUi
{
    class LuaAdapter;
    struct Element;
    struct ScriptSettingsPage
    {
        std::string mName;
        std::string mDescription;
        std::shared_ptr<Element> mElement;
    };
    size_t scriptSettingsPageCount();
    ScriptSettingsPage scriptSettingsPageAt(size_t index);
    void registerSettingsPage(const sol::table& options);
    void clearSettings();
    void attachPageAt(size_t index, LuaAdapter* adapter);
}

#endif // !OPENMW_LUAUI_SCRIPTSETTINGS
