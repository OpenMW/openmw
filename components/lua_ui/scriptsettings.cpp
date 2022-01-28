#include "scriptsettings.hpp"

#include <map>

#include "element.hpp"

namespace LuaUi
{
    namespace
    {
        std::vector<ScriptSettings> allSettings;
    }

    const std::vector<ScriptSettings>& scriptSettings()
    {
        return allSettings;
    }

    void registerSettings(const ScriptSettings& script)
    {
        allSettings.push_back(script);
    }

    void clearSettings()
    {
        allSettings.clear();
    }

    void attachToWidget(size_t index, MyGUI::Widget* widget)
    {
        if (0 <= index && index < allSettings.size())
            allSettings[index].mElement->attachToWidget(widget);
    }
}
