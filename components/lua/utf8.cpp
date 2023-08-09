#include "utf8.hpp"
#include "luastate.hpp"

namespace
{
    static constexpr std::string_view UTF8PATT = "[%z\x01-\x7F\xC2-\xF4][\x80-\xBF]*"; // %z is deprecated in Lua5.2
    static constexpr uint32_t MAXUTF = 0x7FFFFFFFu;
    static constexpr uint32_t MAXUNICODE = 0x10FFFFu;
}

namespace LuaUtf8
{
    sol::table initUtf8Package(sol::state_view& lua)
    {
        sol::table utf8(lua, sol::create);

        utf8["charpattern"] = UTF8PATT;

        return utf8;
    }
}
