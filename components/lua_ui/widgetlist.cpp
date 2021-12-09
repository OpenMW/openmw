#include "widgetlist.hpp"

#include <MyGUI_FactoryManager.h>

#include "widget.hpp"
#include "text.hpp"
#include "textedit.hpp"
#include "window.hpp"

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
}
