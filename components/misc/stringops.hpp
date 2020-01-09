#ifndef MISC_STRINGOPS_H
#define MISC_STRINGOPS_H

#include <cctype>
#include <string>
#include <algorithm>

#include "utf8stream.hpp"

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

    static Utf8Stream::UnicodeChar toLowerUtf8(Utf8Stream::UnicodeChar ch)
    {
        // Russian alphabet
        if (ch >= 0x0410 && ch < 0x0430)
            return ch += 0x20;

        // Cyrillic IO character
        if (ch == 0x0401)
            return ch += 0x50;

        // Latin alphabet
        if (ch >= 0x41 && ch < 0x60)
            return ch += 0x20;

        // Deutch characters
        if (ch == 0xc4 || ch == 0xd6 || ch == 0xdc)
            return ch += 0x20;
        if (ch == 0x1e9e)
            return 0xdf;

        // TODO: probably we will need to support characters from other languages

        return ch;
    }

    static std::string lowerCaseUtf8(const std::string str)
    {
        if (str.empty())
            return str;

        // Decode string as utf8 characters, convert to lower case and pack them to string
        std::string out;
        Utf8Stream stream (str.c_str());
        while (!stream.eof ())
        {
            Utf8Stream::UnicodeChar character = toLowerUtf8(stream.peek());

            if (character <= 0x7f)
                out.append(1, static_cast<char>(character));
            else if (character <= 0x7ff)
            {
                out.append(1, static_cast<char>(0xc0 | ((character >> 6) & 0x1f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }
            else if (character <= 0xffff)
            {
                out.append(1, static_cast<char>(0xe0 | ((character >> 12) & 0x0f)));
                out.append(1, static_cast<char>(0x80 | ((character >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }
            else
            {
                out.append(1, static_cast<char>(0xf0 | ((character >> 18) & 0x07)));
                out.append(1, static_cast<char>(0x80 | ((character >> 12) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | ((character >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }

            stream.consume();
        }

        return out;
    }

    static bool ciLess(const std::string &x, const std::string &y) {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), ci());
    }

    static bool ciEqual(const std::string &x, const std::string &y) {
        if (x.size() != y.size()) {
            return false;
        }
        std::string::const_iterator xit = x.begin();
        std::string::const_iterator yit = y.begin();
        for (; xit != x.end(); ++xit, ++yit) {
            if (toLower(*xit) != toLower(*yit)) {
                return false;
            }
        }
        return true;
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
    static std::string lowerCase(const std::string &in)
    {
        std::string out = in;
        lowerCaseInPlace(out);
        return out;
    }

    struct CiComp
    {
        bool operator()(const std::string& left, const std::string& right) const
        {
            return ciLess(left, right);
        }
    };


    /// Performs a binary search on a sorted container for a string that 'key' starts with
    template<typename Iterator, typename T>
    static Iterator partialBinarySearch(Iterator begin, Iterator end, const T& key)
    {
        const Iterator notFound = end;

        while(begin < end)
        {
            const Iterator middle = begin + (std::distance(begin, end) / 2);

            int comp = Misc::StringUtils::ciCompareLen((*middle), key, (*middle).size());

            if(comp == 0)
                return middle;
            else if(comp > 0)
                end = middle;
            else
                begin = middle + 1;
        }

        return notFound;
    }

    /** @brief Replaces all occurrences of a string in another string.
     *
     * @param str The string to operate on.
     * @param what The string to replace.
     * @param with The replacement string.
     * @param whatLen The length of the string to replace.
     * @param withLen The length of the replacement string.
     *
     * @return A reference to the string passed in @p str.
     */
    static std::string &replaceAll(std::string &str, const char *what, const char *with,
                                   std::size_t whatLen=std::string::npos, std::size_t withLen=std::string::npos)
    {
        if (whatLen == std::string::npos)
            whatLen = strlen(what);

        if (withLen == std::string::npos)
            withLen = strlen(with);

        std::size_t found;
        std::size_t offset = 0;
        while((found = str.find(what, offset, whatLen)) != std::string::npos)
        {
              str.replace(found, whatLen, with, withLen);
              offset = found + withLen;
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

    // TODO: use the std::string_view once we will use the C++17.
    // It should allow us to avoid data copying while we still will support both string and literal arguments.

    static inline void replaceAll(std::string& data, std::string toSearch, std::string replaceStr)
    {
        size_t pos = data.find(toSearch);

        while( pos != std::string::npos)
        {
            data.replace(pos, toSearch.size(), replaceStr);
            pos = data.find(toSearch, pos + replaceStr.size());
        }
    }

     static inline void replaceLast(std::string& str, std::string substr, std::string with)
     {
         size_t pos = str.rfind(substr);
         if (pos == std::string::npos)
             return;

         str.replace(pos, substr.size(), with);
     }
};

}

#endif
