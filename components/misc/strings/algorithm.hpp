#ifndef COMPONENTS_MISC_STRINGS_ALGORITHM_H
#define COMPONENTS_MISC_STRINGS_ALGORITHM_H

#include "lower.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace Misc::StringUtils
{
    struct CiCharLess
    {
        bool operator()(char x, char y) const { return toLower(x) < toLower(y); }
    };

    inline std::string underscoresToSpaces(const std::string_view oldName)
    {
        std::string newName(oldName);
        std::replace(newName.begin(), newName.end(), '_', ' ');
        return newName;
    }

    inline bool ciLess(std::string_view x, std::string_view y)
    {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), CiCharLess());
    }

    inline bool ciEqual(std::string_view x, std::string_view y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(
            std::begin(x), std::end(x), std::begin(y), [](char l, char r) { return toLower(l) == toLower(r); });
    }
    inline bool ciEqual(std::u8string_view x, std::u8string_view y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(
            std::begin(x), std::end(x), std::begin(y), [](char l, char r) { return toLower(l) == toLower(r); });
    }

    inline bool ciStartsWith(std::string_view value, std::string_view prefix)
    {
        return ciEqual(value.substr(0, prefix.size()), prefix);
    }

    inline int ciCompareLen(std::string_view x, std::string_view y, std::size_t len)
    {
        std::string_view::const_iterator xit = x.begin();
        std::string_view::const_iterator yit = y.begin();
        for (; xit != x.end() && yit != y.end() && len > 0; ++xit, ++yit, --len)
        {
            char left = *xit;
            char right = *yit;
            if (left == right)
                continue;

            left = toLower(left);
            right = toLower(right);
            int res = left - right;
            if (res != 0)
                return (res > 0) ? 1 : -1;
        }
        if (len > 0)
        {
            if (xit != x.end())
                return 1;
            if (yit != y.end())
                return -1;
        }
        return 0;
    }

    struct CiEqual
    {
        using is_transparent = void;

        bool operator()(std::string_view left, std::string_view right) const { return ciEqual(left, right); }
    };

    struct CiHash
    {
        using is_transparent = void;

        std::size_t operator()(std::string_view str) const
        {
            // FNV-1a
            std::uint64_t hash{ 0xcbf29ce484222325ull };
            constexpr std::uint64_t prime{ 0x00000100000001B3ull };
            for (char c : str)
            {
                hash ^= static_cast<std::uint64_t>(toLower(c));
                hash *= prime;
            }
            return std::hash<std::uint64_t>()(hash);
        }
    };

    struct CiComp
    {
        using is_transparent = void;

        bool operator()(std::string_view left, std::string_view right) const { return ciLess(left, right); }
    };

    struct StringHash
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
        [[nodiscard]] size_t operator()(const std::string& s) const { return std::hash<std::string>{}(s); }
    };

    /** @brief Replaces all occurrences of a string in another string.
     *
     * @param str The string to operate on.
     * @param what The string to replace.
     * @param with The replacement string.
     * @return A reference to the string passed in @p str.
     */
    inline std::string& replaceAll(std::string& str, std::string_view what, std::string_view with)
    {
        std::size_t found;
        std::size_t offset = 0;
        while ((found = str.find(what, offset)) != std::string::npos)
        {
            str.replace(found, what.size(), with);
            offset = found + with.size();
        }
        return str;
    }
    inline std::u8string& replaceAll(std::u8string& str, std::u8string_view what, std::u8string_view with)
    {
        std::size_t found;
        std::size_t offset = 0;
        while ((found = str.find(what, offset)) != std::u8string::npos)
        {
            str.replace(found, what.size(), with);
            offset = found + with.size();
        }
        return str;
    }

    inline bool ciEndsWith(std::string_view s, std::string_view suffix)
    {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(), [](char l, char r) {
            return toLower(l) == toLower(r);
        });
    }
    inline bool ciEndsWith(std::u8string_view s, std::u8string_view suffix)
    {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(), [](char l, char r) {
            return toLower(l) == toLower(r);
        });
    }

    inline void trim(std::string& s)
    {
        const auto notSpace = [](char ch) {
            // TODO Do we care about multibyte whitespace?
            return !std::isspace(static_cast<unsigned char>(ch));
        };
        // left trim
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));

        // right trim
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    }

    template <class Container>
    void split(std::string_view str, Container& cont, std::string_view delims = " ")
    {
        std::size_t current = str.find_first_of(delims);
        std::size_t previous = 0;
        while (current != std::string::npos)
        {
            cont.emplace_back(str.substr(previous, current - previous));
            previous = current + 1;
            current = str.find_first_of(delims, previous);
        }
        cont.emplace_back(str.substr(previous, current - previous));
    }

    inline void replaceLast(std::string& str, std::string_view substr, std::string_view with)
    {
        const std::size_t pos = str.rfind(substr);
        if (pos == std::string::npos)
            return;
        str.replace(pos, substr.size(), with);
    }

    inline std::string_view::size_type ciFind(std::string_view str, std::string_view substr)
    {
        if (str.size() < substr.size())
            return std::string_view::npos;
        for (std::string_view::size_type i = 0, n = str.size() - substr.size() + 1; i < n; ++i)
            if (ciEqual(str.substr(i, substr.size()), substr))
                return i;
        return std::string_view::npos;
    }
}

#endif
