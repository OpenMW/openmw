#include "util.hpp"

#include <MyGUI_FactoryManager.h>

#include "adapter.hpp"
#include "widget.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "window.hpp"
#include "image.hpp"
#include "container.hpp"

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
        MyGUI::FactoryManager::getInstance().registerFactory<LuaContainer>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaTileRect>("BasisSkin");
    }

    const std::unordered_map<std::string, std::string>& widgetTypeToName()
    {
        static std::unordered_map<std::string, std::string> types{
            { "LuaWidget", "Widget" },
            { "LuaText", "Text" },
            { "LuaTextEdit", "TextEdit" },
            { "LuaWindow", "Window" },
            { "LuaImage", "Image" },
        };
        return types;
    }

    void clearUserInterface()
    {
        clearSettings();
        while (!Element::sAllElements.empty())
            Element::sAllElements.begin()->second->destroy();
    }
}
