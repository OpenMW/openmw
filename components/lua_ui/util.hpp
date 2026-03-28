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

    bool warnUnused(std::vector<std::string>& warnings, sol::object table, const std::string& tableName,
        const std::set<std::string_view>& usedKeys, bool generateWarningStrings);
}

#endif // OPENMW_LUAUI_WIDGETLIST
