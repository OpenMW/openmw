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

    void attachToWidget(const ScriptSettings& script, MyGUI::Widget* widget)
    {
        WidgetExtension* root = script.mElement->mRoot;
        if (!root)
            return;
        root->widget()->attachToWidget(widget);
        root->updateCoord();
    }
}
