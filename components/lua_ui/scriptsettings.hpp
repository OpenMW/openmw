#ifndef OPENMW_LUAUI_SCRIPTSETTINGS
#define OPENMW_LUAUI_SCRIPTSETTINGS

#include <vector>
#include <string>
#include <string_view>

#include <MyGUI_Widget.h>

namespace LuaUi
{
    struct Element;
    struct ScriptSettingsPage
    {
        std::string mName;
        std::string mDescription;
        Element* mElement; // TODO: figure out if this can lead to use after free
    };
    size_t scriptSettingsPageCount();
    ScriptSettingsPage scriptSettingsPageAt(size_t index);
    void registerSettingsPage(const sol::table& options);
    void clearSettings();
    void attachToWidget(size_t index, MyGUI::Widget* widget = nullptr);
}

#endif // !OPENMW_LUAUI_SCRIPTSETTINGS
