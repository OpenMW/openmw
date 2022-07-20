#include "scriptsettings.hpp"

#include <map>
#include <sol/sol.hpp>

#include "registerscriptsettings.hpp"
#include "element.hpp"
#include "adapter.hpp"

namespace LuaUi
{
    namespace
    {
        std::vector<sol::table> allPages;
        ScriptSettingsPage parse(const sol::table& options)
        {
            auto name = options.get_or("name", std::string());
            auto searchHints = options.get_or("searchHints", std::string());
            auto element = options.get_or<std::shared_ptr<LuaUi::Element>>("element", nullptr);
            if (name.empty())
                Log(Debug::Warning) << "A script settings page has an empty name";
            if (!element.get())
                Log(Debug::Warning) << "A script settings page has no UI element assigned";
            return {
                name, searchHints, element
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

    void attachPageAt(size_t index, LuaAdapter* adapter)
    {
        if (index < allPages.size())
        {
            ScriptSettingsPage page = parse(allPages[index]);
            adapter->detach();
            if (page.mElement.get())
                adapter->attach(page.mElement);
        }
    }
}
