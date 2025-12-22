#ifndef COMPONENTS_LUAUI_CONTENT
#define COMPONENTS_LUAUI_CONTENT

#include <sol/sol.hpp>

#include <components/lua/luastate.hpp>

namespace LuaUi
{
    sol::protected_function loadContentConstructor(LuaUtil::LuaState* state);

    bool isValidContent(const sol::object& object);

    class ContentView
    {
    public:
        // accepts only Lua tables returned by ui.content
        explicit ContentView(sol::main_table table)
            : mTable(std::move(table))
        {
            if (!isValidContent(mTable))
                throw std::domain_error("Invalid UI Content");
        }

        size_t size() const { return mTable.size(); }

        sol::object at(size_t index) const
        {
            if (index < size())
                return mTable.get<sol::object>(toLua(index));
            else
                throw std::range_error("Invalid Content index");
        }

    private:
        sol::main_table mTable;

        static inline size_t toLua(size_t index) { return index + 1; }
    };
}

#endif // COMPONENTS_LUAUI_CONTENT
