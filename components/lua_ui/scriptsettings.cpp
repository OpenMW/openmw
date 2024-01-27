#include "scriptsettings.hpp"

#include <map>
#include <sol/sol.hpp>

#include "adapter.hpp"
#include "element.hpp"
#include "registerscriptsettings.hpp"

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
            return { std::move(name), std::move(searchHints), std::move(element) };
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

    void removeSettingsPage(const sol::table& options)
    {
        std::erase_if(allPages, [options](const sol::table& it) { return it == options; });
    }

    void clearSettings()
    {
        allPages.clear();
    }

    void attachPageAt(size_t index, LuaAdapter* adapter)
    {
        adapter->detach();
        if (index < allPages.size())
        {
            ScriptSettingsPage page = parse(allPages[index]);
            if (page.mElement.get())
                adapter->attach(page.mElement);
        }
    }
}
