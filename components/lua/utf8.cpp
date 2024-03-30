#include <codecvt>
#include <components/misc/strings/format.hpp>

#include "utf8.hpp"

namespace
{
    constexpr std::string_view UTF8PATT = "[%z\x01-\x7F\xC2-\xF4][\x80-\xBF]*"; // %z is deprecated in Lua5.2
    constexpr uint32_t MAXUTF = 0x7FFFFFFFu;
    // constexpr uint32_t MAXUNICODE = 0x10FFFFu;

    inline bool isNilOrNone(const sol::stack_proxy arg)
    {
        return (arg.get_type() == sol::type::lua_nil || arg.get_type() == sol::type::none);
    }

    inline std::int64_t getInteger(const sol::stack_proxy arg, const size_t n, std::string_view name)
    {
        double integer;
        if (!arg.is<double>())
            throw std::runtime_error(Misc::StringUtils::format("bad argument #%i to '%s' (number expected, got %s)", n,
                name, sol::type_name(arg.lua_state(), arg.get_type())));

        if (std::modf(arg, &integer) != 0)
            throw std::runtime_error(
                Misc::StringUtils::format("bad argument #%i to '%s' (number has no integer representation)", n, name));

        return static_cast<std::int64_t>(integer);
    }

    // If the input 'pos' is negative, it is treated as counting from the end of the string,
    // where -1 represents the last character position, -2 represents the second-to-last position,
    // and so on. If 'pos' is non-negative, it is used as-is.
    inline void relativePosition(int64_t& pos, const size_t len)
    {
        if (pos < 0)
            pos = std::max<int64_t>(0, pos + len + 1);
    }

    // returns: first - character pos in bytes, second - character codepoint
    std::pair<int64_t, int64_t> decodeNextUTF8Character(std::string_view s, std::vector<int64_t>& pos_byte)
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
                    throw std::runtime_error(
                        "bad argument #" + std::to_string(i + 1) + " to 'char' (value out of range)");

                // this feels dodgy if wchar_t is 16-bit as MAXUTF won't fit in sixteen bits
                result += converter.to_bytes(static_cast<wchar_t>(codepoint));
            }
            return result;
        };

        utf8["codes"] = [](std::string_view s) {
            return sol::as_function(
                [s, pos_byte = std::vector<int64_t>{ 1 }]() mutable -> sol::optional<std::pair<int64_t, int64_t>> {
                    if (pos_byte.back() <= static_cast<int64_t>(s.size()))
                    {
                        const auto pair = decodeNextUTF8Character(s, pos_byte);
                        if (pair.second == -1)
                            throw std::runtime_error(
                                "Invalid UTF-8 code at position " + std::to_string(pos_byte.size()));

                        return pair;
                    }
                    return sol::nullopt;
                });
        };

        utf8["len"] = [](std::string_view s,
                          const sol::variadic_args args) -> std::variant<size_t, std::pair<sol::object, int64_t>> {
            const size_t len = s.size();
            int64_t iv = isNilOrNone(args[0]) ? 1 : getInteger(args[0], 2, "len");
            int64_t fv = isNilOrNone(args[1]) ? -1 : getInteger(args[1], 3, "len");

            relativePosition(iv, len);
            relativePosition(fv, len);

            if (iv <= 0)
                throw std::runtime_error("bad argument #2 to 'len' (initial position out of bounds)");
            if (fv > static_cast<int64_t>(len))
                throw std::runtime_error("bad argument #3 to 'len' (final position out of bounds)");

            if (len == 0)
                return len;

            std::vector<int64_t> pos_byte = { iv };

            while (pos_byte.back() <= fv)
            {
                if (decodeNextUTF8Character(s, pos_byte).second == -1)
                    return std::pair(sol::lua_nil, pos_byte.back());
            }
            return pos_byte.size() - 1;
        };

        utf8["codepoint"]
            = [](std::string_view s, const sol::variadic_args args) -> sol::as_returns_t<std::vector<int64_t>> {
            size_t len = s.size();
            int64_t iv = isNilOrNone(args[0]) ? 1 : getInteger(args[0], 2, "codepoint");
            int64_t fv = isNilOrNone(args[1]) ? iv : getInteger(args[1], 3, "codepoint");

            relativePosition(iv, len);
            relativePosition(fv, len);

            if (iv <= 0)
                throw std::runtime_error("bad argument #2 to 'codepoint' (initial position out of bounds)");
            if (fv > static_cast<int64_t>(len))
                throw std::runtime_error("bad argument #3 to 'codepoint' (final position out of bounds)");

            if (iv > fv)
                return sol::as_returns(std::vector<int64_t>{}); /* empty interval; return nothing */

            std::vector<int64_t> pos_byte = { iv };
            std::vector<int64_t> codepoints;

            while (pos_byte.back() <= fv)
            {
                codepoints.push_back(decodeNextUTF8Character(s, pos_byte).second);
                if (codepoints.back() == -1)
                    throw std::runtime_error("Invalid UTF-8 code at position " + std::to_string(pos_byte.size()));
            }

            return sol::as_returns(std::move(codepoints));
        };

        utf8["offset"]
            = [](std::string_view s, const int64_t n, const sol::variadic_args args) -> sol::optional<int64_t> {
            size_t len = s.size();
            int64_t iv;

            if (isNilOrNone(args[0]))
            {
                if (n >= 0)
                    iv = 1;
                else
                    iv = s.size() + 1;
            }
            else
                iv = getInteger(args[0], 3, "offset");

            std::vector<int64_t> pos_byte = { 1 };

            relativePosition(iv, len);

            if (iv > static_cast<int64_t>(len) + 1)
                throw std::runtime_error("bad argument #3 to 'offset' (position out of bounds)");

            while (pos_byte.back() <= static_cast<int64_t>(len))
                decodeNextUTF8Character(s, pos_byte);

            for (auto it = pos_byte.begin(); it != pos_byte.end(); ++it)
            {
                if (*it == iv)
                {
                    if (n <= 0 && it + n >= pos_byte.begin())
                        return *(it + n);
                    if (n > 0 && it + n - 1 < pos_byte.end())
                        return *(it + n - 1);
                    break;
                }
                else if (*it > iv) /* a continuation byte */
                {
                    if (n == 0)
                        return *(it - 1); /* special case */
                    else
                        throw std::runtime_error("initial position is a continuation byte");
                }
            }

            return sol::nullopt;
        };

        return utf8;
    }
}
