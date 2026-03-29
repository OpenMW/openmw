#include "util.hpp"

#include <ranges>

#include <MyGUI_FactoryManager.h>

#include "adapter.hpp"
#include "container.hpp"
#include "flex.hpp"
#include "image.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "widget.hpp"
#include "window.hpp"

#include "element.hpp"

namespace LuaUi
{

    void registerAllWidgets()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<LuaAdapter>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaWidget>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaText>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaTextEdit>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaWindow>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaImage>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaTileRect>("BasisSkin");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaContainer>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaFlex>("Widget");
    }

    const std::unordered_map<std::string, std::string>& widgetTypeToName()
    {
        static std::unordered_map<std::string, std::string> types{
            { "LuaWidget", "Widget" },
            { "LuaText", "Text" },
            { "LuaTextEdit", "TextEdit" },
            { "LuaWindow", "Window" },
            { "LuaImage", "Image" },
            { "LuaFlex", "Flex" },
            { "LuaContainer", "Container" },
        };
        return types;
    }

    void clearGameInterface()
    {
        while (!Element::sGameElements.empty())
            Element::erase(Element::sGameElements.begin()->second.get());
    }

    void clearMenuInterface()
    {
        while (!Element::sMenuElements.empty())
            Element::erase(Element::sMenuElements.begin()->second.get());
    }

    bool warnUnused(std::vector<std::string>& warnings, sol::object object, const std::string& tableName,
        const std::vector<std::string_view>& usedKeys, bool generateWarningStrings)
    {
        auto beginningSize = warnings.size();
        if (!object.is<sol::table>())
            return false;
        sol::table table = object.as<sol::table>();
        for (const auto& [key, value] : table)
        {
            if (!key.is<std::string>())
                continue;
            auto keyStr = key.as<std::string>();

            if (std::ranges::find(usedKeys, keyStr) == usedKeys.end())
            {
                if (!generateWarningStrings)
                    return true;
                warnings.push_back("unused key '" + keyStr + "' in " + tableName);
            }
        }
        return beginningSize != warnings.size();
    }
}
