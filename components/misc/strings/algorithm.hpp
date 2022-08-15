#ifndef COMPONENTS_MISC_STRINGS_ALGORITHM_H
#define COMPONENTS_MISC_STRINGS_ALGORITHM_H

#include "lower.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>

namespace Misc::StringUtils
{
    struct CiCharLess
    {
        bool operator()(char x, char y) const { return toLower(x) < toLower(y); }
    };

    inline bool ciLess(std::string_view x, std::string_view y)
    {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), CiCharLess());
    }

    inline bool ciEqual(std::string_view x, std::string_view y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(std::begin(x), std::end(x), std::begin(y),
                          [] (char l, char r) { return toLower(l) == toLower(r); });
    }

    inline int ciCompareLen(std::string_view x, std::string_view y, std::size_t len)
    {
        std::string_view::const_iterator xit = x.begin();
        std::string_view::const_iterator yit = y.begin();
        for(; xit != x.end() && yit != y.end() && len > 0; ++xit, ++yit, --len)
        {
            char left = *xit;
            char right = *yit;
            if (left == right)
                continue;

            left = toLower(left);
            right = toLower(right);
            int res = left - right;
            if(res != 0)
                return (res > 0) ? 1 : -1;
        }
        if(len > 0)
        {
            if(xit != x.end())
                return 1;
            if(yit != y.end())
                return -1;
        }
        return 0;
    }

    struct CiEqual
    {
        using is_transparent = void;

        bool operator()(std::string_view left, std::string_view right) const
        {
            return ciEqual(left, right);
        }
    };

    struct CiHash
    {
        using is_transparent = void;

        constexpr std::size_t operator()(std::string_view str) const
        {
            // FNV-1a
            std::size_t hash{0xcbf29ce484222325ull};
            constexpr std::size_t prime{0x00000100000001B3ull};
            for(char c : str)
            {
                hash ^= static_cast<std::size_t>(toLower(c));
                hash *= prime;
            }
            return hash;
        }
    };

    struct CiComp
    {
        bool operator()(std::string_view left, std::string_view right) const
        {
            return ciLess(left, right);
        }
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

    inline bool ciEndsWith(std::string_view s, std::string_view suffix)
    {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(),
                                                       [](char l, char r) { return toLower(l) == toLower(r); });
    }

    inline void trim(std::string& s)
    {
        const auto notSpace = [](char ch)
        {
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
}

#endif
