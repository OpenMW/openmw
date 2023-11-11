#ifndef COMPONENTS_LUAUI_CONTENT
#define COMPONENTS_LUAUI_CONTENT

#include <map>
#include <string>

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
        explicit ContentView(sol::table table)
            : mTable(std::move(table))
        {
            if (!isValidContent(mTable))
                throw std::domain_error("Invalid UI Content");
        }

        size_t size() const { return mTable.size(); }

        void assign(std::string_view name, const sol::table& table)
        {
            if (indexOf(name).has_value())
                mTable[name] = table;
            else
                throw std::domain_error("Invalid Content key");
        }
        void assign(size_t index, const sol::table& table)
        {
            if (index <= size())
                mTable[toLua(index)] = table;
            else
                throw std::range_error("Invalid Content index");
        }
        void insert(size_t index, const sol::table& table) { callMethod("insert", toLua(index), table); }

        sol::object at(size_t index) const
        {
            if (index < size())
                return mTable.get<sol::object>(toLua(index));
            else
                throw std::range_error("Invalid Content index");
        }
        sol::object at(std::string_view name) const
        {
            if (indexOf(name).has_value())
                return mTable.get<sol::object>(name);
            else
                throw std::range_error("Invalid Content key");
        }
        void remove(size_t index)
        {
            if (index < size())
                // for some reason mTable[key] = value doesn't call __newindex
                getMetatable()[sol::meta_function::new_index].get<sol::protected_function>()(
                    mTable, toLua(index), sol::nil);
            else
                throw std::range_error("Invalid Content index");
        }
        void remove(std::string_view name)
        {
            auto index = indexOf(name);
            if (index.has_value())
                remove(index.value());
            else
                throw std::domain_error("Invalid Content key");
        }
        std::optional<size_t> indexOf(std::string_view name) const
        {
            sol::object result = callMethod("indexOf", name);
            if (result.is<size_t>())
                return fromLua(LuaUtil::cast<size_t>(result));
            else
                return std::nullopt;
        }
        std::optional<size_t> indexOf(const sol::table& table) const
        {
            sol::object result = callMethod("indexOf", table);
            if (result.is<size_t>())
                return fromLua(LuaUtil::cast<size_t>(result));
            else
                return std::nullopt;
        }

        sol::table getMetatable() const { return mTable[sol::metatable_key].get<sol::table>(); }

    private:
        sol::table mTable;

        template <typename... Arg>
        sol::object callMethod(std::string_view name, Arg&&... arg) const
        {
            return mTable.get<sol::protected_function>(name)(mTable, arg...);
        }

        static inline size_t toLua(size_t index) { return index + 1; }
        static inline size_t fromLua(size_t index) { return index - 1; }
    };
}

#endif // COMPONENTS_LUAUI_CONTENT
