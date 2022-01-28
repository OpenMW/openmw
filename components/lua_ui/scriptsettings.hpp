#ifndef OPENMW_LUAUI_SCRIPTSETTINGS
#define OPENMW_LUAUI_SCRIPTSETTINGS

#include <vector>
#include <string>
#include <string_view>

#include <MyGUI_Widget.h>

namespace LuaUi
{
    struct Element;
    struct ScriptSettings
    {
        std::string mName;
        std::string mSearchHints;
        Element* mElement; // TODO: figure out if this can lead to use after free
    };
    const std::vector<ScriptSettings>& scriptSettings();
    void registerSettings(const ScriptSettings& script);
    void clearSettings();
    void attachToWidget(size_t index, MyGUI::Widget* widget = nullptr);
}

#endif // !OPENMW_LUAUI_SCRIPTSETTINGS
