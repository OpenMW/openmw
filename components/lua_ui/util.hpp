#ifndef OPENMW_LUAUI_WIDGETLIST
#define OPENMW_LUAUI_WIDGETLIST

#include <set>
#include <sol/table.hpp>
#include <string>
#include <unordered_map>

namespace LuaUi
{
    void registerAllWidgets();

    const std::unordered_map<std::string, std::string>& widgetTypeToName();

    void clearGameInterface();
    void clearMenuInterface();

    void warnUnused(std::vector<std::string>& warnings, sol::object table, const std::string& tableName,
        std::set<std::string_view> usedKeys);
}

#endif // OPENMW_LUAUI_WIDGETLIST
