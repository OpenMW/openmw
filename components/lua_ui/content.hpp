#ifndef COMPONENTS_LUAUI_CONTENT
#define COMPONENTS_LUAUI_CONTENT

#include <map>
#include <string>

#include <sol/sol.hpp>

namespace LuaUi
{
    class Content
    {
    public:
        using iterator = std::vector<sol::table>::iterator;

        Content() { sInstanceCount++; }
        ~Content() { sInstanceCount--; }
        Content(const Content& c)
        {
            this->mNamed = c.mNamed;
            this->mOrdered = c.mOrdered;
            sInstanceCount++;
        }
        Content(Content&& c)
        {
            this->mNamed = std::move(c.mNamed);
            this->mOrdered = std::move(c.mOrdered);
            sInstanceCount++;
        }

        // expects a Lua array - a table with keys from 1 to n without any nil values in between
        // any other keys are ignored
        explicit Content(const sol::table&);

        size_t size() const { return mOrdered.size(); }

        void assign(std::string_view name, const sol::table& table);
        void assign(size_t index, const sol::table& table);
        void insert(size_t index, const sol::table& table);

        sol::table at(size_t index) const;
        sol::table at(std::string_view name) const;
        size_t remove(size_t index);
        size_t remove(std::string_view name);
        size_t indexOf(const sol::table& table) const;

        static int64_t getInstanceCount() { return sInstanceCount; }

    private:
        std::map<std::string, size_t, std::less<>> mNamed;
        std::vector<sol::table> mOrdered;
        static int64_t sInstanceCount; // debug information, shown in Lua profiler
    };

}

#endif // COMPONENTS_LUAUI_CONTENT
