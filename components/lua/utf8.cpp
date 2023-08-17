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

    // returns: first - character pos in bytes, second - character codepoint
    static std::pair<int64_t, int64_t> poscodes(const std::string_view& s, std::vector<int64_t>& pos_byte)
    {
        const int64_t pos = pos_byte.back() - 1;
        const unsigned char ch = static_cast<unsigned char>(s[pos]);
        int64_t codepoint = -1;
        size_t byteSize = 0;

        if ((ch & 0b10000000) == 0)
        {
            codepoint = ch;
            byteSize = 1;
        }
        else if ((ch & 0b11100000) == 0b11000000)
        {
            codepoint = ch & 0b00011111;
            byteSize = 2;
        }
        else if ((ch & 0b11110000) == 0b11100000)
        {
            codepoint = ch & 0b00001111;
            byteSize = 3;
        }
        else if ((ch & 0b11111000) == 0b11110000)
        {
            codepoint = ch & 0b00000111;
            byteSize = 4;
        }

        // construct codepoint for non-ascii
        for (size_t i = 1; i < byteSize; ++i)
        {
            // if not a continuation byte
            if ((pos + i) >= s.size() || (static_cast<unsigned char>(s[pos + i]) & 0b11000000) != 0b10000000)
            {
                return std::make_pair(0, -1);
            }
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(s[pos + i]) & 0b00111111);
        }

        std::pair<size_t, int64_t> res = std::make_pair(pos_byte.back(), codepoint);

        pos_byte.push_back(pos_byte.back() + byteSize); /* the next character (if exists) starts at this byte */

        return res;
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

        utf8["codes"] = [pos_byte = std::vector<int64_t>{ 1 }](const std::string_view& s) {
            return sol::as_function([s, pos_byte]() mutable -> sol::optional<std::pair<int64_t, int64_t>> {
                if (pos_byte.back() <= static_cast<int64_t>(s.size()))
                {
                    const auto pair = poscodes(s, pos_byte);
                    if (pair.second == -1)
                        throw std::runtime_error("Invalid UTF-8 code at position " + std::to_string(pos_byte.size()));

                    return pair;
                }
                return sol::nullopt;
            });
        };
        return utf8;
    }
}
