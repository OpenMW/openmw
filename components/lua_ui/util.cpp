#include "util.hpp"

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
#include "registerscriptsettings.hpp"

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
}
