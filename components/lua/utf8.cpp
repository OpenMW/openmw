#include <codecvt>

#include "utf8.hpp"

namespace
{
    static constexpr std::string_view UTF8PATT = "[%z\x01-\x7F\xC2-\xF4][\x80-\xBF]*"; // %z is deprecated in Lua5.2
    static constexpr uint32_t MAXUTF = 0x7FFFFFFFu;
    static constexpr uint32_t MAXUNICODE = 0x10FFFFu;

    inline static double getInteger(const sol::stack_proxy arg, const size_t& n, const std::string_view& name)
    {
        double integer;
        if (!arg.is<double>())
            throw std::runtime_error(std::format("bad argument #{} to '{}' (number expected, got {})", n, name,
                sol::type_name(arg.lua_state(), arg.get_type())));

        if (std::modf(arg, &integer) != 0)
            throw std::runtime_error(
                std::format("bad argument #{} to '{}' (number has no integer representation)", n, name));

        return integer;
    }

}

namespace LuaUtf8
{
    sol::table initUtf8Package(sol::state_view& lua)
    {
        sol::table utf8(lua, sol::create);

        utf8["charpattern"] = UTF8PATT;

        utf8["char"] = [](const sol::variadic_args args) -> std::string {
            std::string result{};
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            for (size_t i = 0; i < args.size(); ++i)
            {
                int64_t codepoint = getInteger(args[i], (i + 1), "char");
                if (codepoint < 0 || codepoint > MAXUTF)
                    throw std::runtime_error(std::format("bad argument #{} to 'char' (value out of range)", (i + 1)));

                result += converter.to_bytes(codepoint);
            }
            return result;
        };
        return utf8;
    }
}
