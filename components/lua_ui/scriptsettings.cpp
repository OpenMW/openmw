#include "scriptsettings.hpp"

#include <map>

#include "element.hpp"

namespace LuaUi
{
    namespace
    {
        std::vector<sol::table> allPages;
        ScriptSettingsPage parse(const sol::table& options)
        {
            auto name = options.get_or("name", std::string());
            auto description = options.get_or("description", std::string());
            auto element = options.get_or<std::shared_ptr<LuaUi::Element>>("element", nullptr);
            if (name.empty())
                Log(Debug::Warning) << "A script settings page has an empty name";
            if (!element.get())
                Log(Debug::Warning) << "A script settings page has no UI element assigned";
            return {
                name, description, element.get()
            };
        }
    }

    size_t scriptSettingsPageCount()
    {
        return allPages.size();
    }

    ScriptSettingsPage scriptSettingsPageAt(size_t index)
    {
        return parse(allPages[index]);
    }

    void registerSettingsPage(const sol::table& options)
    {
        allPages.push_back(options);
    }

    void clearSettings()
    {
        allPages.clear();
    }

    void attachToWidget(size_t index, MyGUI::Widget* widget)
    {
        if (index < allPages.size())
        {
            ScriptSettingsPage page = parse(allPages[index]);
            if (page.mElement)
                page.mElement->attachToWidget(widget);
        }
    }
}
