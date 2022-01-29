#include "scriptsettings.hpp"

#include <map>

#include "element.hpp"

namespace LuaUi
{
    namespace
    {
        std::vector<ScriptSettingsPage> allPages;
    }

    const std::vector<ScriptSettingsPage>& scriptSettingsPages()
    {
        return allPages;
    }

    void registerSettingsPage(const ScriptSettingsPage& page)
    {
        allPages.push_back(page);
    }

    void clearSettings()
    {
        allPages.clear();
    }

    void attachToWidget(size_t index, MyGUI::Widget* widget)
    {
        if (index < allPages.size())
            allPages[index].mElement->attachToWidget(widget);
    }
}
