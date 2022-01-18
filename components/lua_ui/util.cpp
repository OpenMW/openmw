#include "util.hpp"

#include <MyGUI_FactoryManager.h>

#include "widget.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "window.hpp"

#include "element.hpp"

namespace LuaUi
{

    void registerAllWidgets()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<LuaWidget>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaText>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaTextEdit>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<LuaWindow>("Widget");
    }

    const std::unordered_map<std::string, std::string>& widgetTypeToName()
    {
        static std::unordered_map<std::string, std::string> types{
            { "LuaWidget", "Widget" },
            { "LuaText", "Text" },
            { "LuaTextEdit", "TextEdit" },
            { "LuaWindow", "Window" },
        };
        return types;
    }

    void clearUserInterface()
    {
        while (!Element::sAllElements.empty())
            Element::sAllElements.begin()->second->destroy();
    }
}
