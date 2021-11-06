#ifndef MISC_STRINGOPS_H
#define MISC_STRINGOPS_H

#include <cctype>
#include <string>
#include <algorithm>
#include <string_view>
#include <iterator>
#include <functional>

namespace Misc
{
class StringUtils
{
    struct ci
    {
        bool operator()(char x, char y) const {
            return toLower(x) < toLower(y);
        }
    };

    // Allow to convert complex arguments to C-style strings for format() function
    template <typename T>
    static T argument(T value) noexcept
    {
        static_assert(!std::is_same_v<T, std::string_view>, "std::string_view is not supported");
        return value;
    }

    template <typename T>
    static T const * argument(std::basic_string<T> const & value) noexcept
    {
        return value.c_str();
    }

public:

    /// Plain and simple locale-unaware toLower. Anything from A to Z is lower-cased, multibyte characters are unchanged.
    /// Don't use std::tolower(char, locale&) because that is abysmally slow.
    /// Don't use tolower(int) because that depends on global locale.
    static char toLower(char c)
    {
        return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : c;
    }

    static bool ciLess(const std::string &x, const std::string &y) {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), ci());
    }

    template <class X, class Y>
    static bool ciEqual(const X& x, const Y& y)
    {
        if (std::size(x) != std::size(y))
            return false;
        return std::equal(std::begin(x), std::end(x), std::begin(y),
                          [] (char l, char r) { return toLower(l) == toLower(r); });
    }

    template <std::size_t n>
    static auto ciEqual(const char(& x)[n], const char(& y)[n])
    {
        static_assert(n > 0);
        return ciEqual(std::string_view(x, n - 1), std::string_view(y, n - 1));
    }

    template <std::size_t n, class T>
    static auto ciEqual(const char(& x)[n], const T& y)
    {
        static_assert(n > 0);
        return ciEqual(std::string_view(x, n - 1), y);
    }

    template <std::size_t n, class T>
    static auto ciEqual(const T& x, const char(& y)[n])
    {
        static_assert(n > 0);
        return ciEqual(x, std::string_view(y, n - 1));
    }

    static int ciCompareLen(const std::string &x, const std::string &y, size_t len)
    {
        std::string::const_iterator xit = x.begin();
        std::string::const_iterator yit = y.begin();
        for(;xit != x.end() && yit != y.end() && len > 0;++xit,++yit,--len)
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

    /// Transforms input string to lower case w/o copy
    static void lowerCaseInPlace(std::string &inout) {
        for (unsigned int i=0; i<inout.size(); ++i)
            inout[i] = toLower(inout[i]);
    }

    /// Returns lower case copy of input string
    static std::string lowerCase(std::string_view in)
    {
        std::string out(in);
        lowerCaseInPlace(out);
        return out;
    }

    struct CiEqual
    {
        bool operator()(const std::string& left, const std::string& right) const
        {
            return ciEqual(left, right);
        }
    };
    struct CiHash
    {
        std::size_t operator()(std::string str) const
        {
            lowerCaseInPlace(str);
            return std::hash<std::string>{}(str);
        }
    };
    struct CiComp
    {
        bool operator()(const std::string& left, const std::string& right) const
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
    static std::string &replaceAll(std::string &str, std::string_view what, std::string_view with)
    {
        std::size_t found;
        std::size_t offset = 0;
        while((found = str.find(what, offset)) != std::string::npos)
        {
              str.replace(found, what.size(), with);
              offset = found + with.size();
        }
        return str;
    }

    // Requires some C++11 features:
    // 1. std::string needs to be contiguous
    // 2. std::snprintf with zero size (second argument) returns an output string size
    // 3. variadic templates support
    template <typename ... Args>
    static std::string format(const char* fmt, Args const & ... args)
    {
        auto size = std::snprintf(nullptr, 0, fmt, argument(args) ...);
        // Note: sprintf also writes a trailing null character. We should remove it.
        std::string ret(size+1, '\0');
        std::sprintf(&ret[0], fmt, argument(args) ...);
        ret.erase(size);

        return ret;
    }

    template <typename ... Args>
    static std::string format(const std::string& fmt, Args const & ... args)
    {
        return format(fmt.c_str(), args ...);
    }

    static inline void trim(std::string &s)
    {
        // left trim
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));

        // right trim
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
        {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    template <class Container>
    static inline void split(const std::string& str, Container& cont, const std::string& delims = " ")
    {
        std::size_t current, previous = 0;
        current = str.find_first_of(delims);
        while (current != std::string::npos)
        {
            cont.push_back(str.substr(previous, current - previous));
            previous = current + 1;
            current = str.find_first_of(delims, previous);
        }
        cont.push_back(str.substr(previous, current - previous));
    }

    static inline void replaceLast(std::string& str, const std::string& substr, const std::string& with)
    {
        size_t pos = str.rfind(substr);
        if (pos == std::string::npos)
            return;
        str.replace(pos, substr.size(), with);
    }

    static inline bool ciEndsWith(std::string_view s, std::string_view suffix)
    {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(),
                                                       [](char l, char r) { return toLower(l) == toLower(r); });
    };
};

}

#endif
