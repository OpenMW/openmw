#ifndef OPENMW_LUAUI_WIDGETLIST
#define OPENMW_LUAUI_WIDGETLIST

#include <unordered_map>
#include <string>

namespace LuaUi
{
    void registerAllWidgets();

    const std::unordered_map<std::string, std::string>& widgetTypeToName();

    void clearUserInterface();
}

#endif // OPENMW_LUAUI_WIDGETLIST
