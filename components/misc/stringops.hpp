#ifndef MISC_STRINGOPS_H
#define MISC_STRINGOPS_H

#include <cctype>
#include <cstring>
#include <string>
#include <algorithm>

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

public:

    /// Plain and simple locale-unaware toLower. Anything from A to Z is lower-cased, multibyte characters are unchanged.
    /// Don't use std::tolower(char, locale&) because that is abysmally slow.
    /// Don't use tolower(int) because that depends on global locale.
    static char toLower(char c)
    {
        switch(c)
        {
        case 'A':return 'a';
        case 'B':return 'b';
        case 'C':return 'c';
        case 'D':return 'd';
        case 'E':return 'e';
        case 'F':return 'f';
        case 'G':return 'g';
        case 'H':return 'h';
        case 'I':return 'i';
        case 'J':return 'j';
        case 'K':return 'k';
        case 'L':return 'l';
        case 'M':return 'm';
        case 'N':return 'n';
        case 'O':return 'o';
        case 'P':return 'p';
        case 'Q':return 'q';
        case 'R':return 'r';
        case 'S':return 's';
        case 'T':return 't';
        case 'U':return 'u';
        case 'V':return 'v';
        case 'W':return 'w';
        case 'X':return 'x';
        case 'Y':return 'y';
        case 'Z':return 'z';
        default:return c;
        };
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
            int res = *xit - *yit;
            if(res != 0 && toLower(*xit) != toLower(*yit))
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
};

}

#endif
