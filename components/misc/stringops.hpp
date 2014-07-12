#ifndef MISC_STRINGOPS_H
#define MISC_STRINGOPS_H

#include <cctype>
#include <string>
#include <algorithm>
#include <locale>

namespace Misc
{
class StringUtils
{

    static std::locale mLocale;
    struct ci
    {
        bool operator()(char x, char y) const {
            return std::tolower(x, StringUtils::mLocale) < std::tolower(y, StringUtils::mLocale);
        }
    };

public:
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
            if (std::tolower(*xit, mLocale) != std::tolower(*yit, mLocale)) {
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
            if(res != 0 && std::tolower(*xit, mLocale) != std::tolower(*yit, mLocale))
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
    static std::string &toLower(std::string &inout) {
        for (unsigned int i=0; i<inout.size(); ++i)
            inout[i] = std::tolower(inout[i], mLocale);
        return inout;
    }

    /// Returns lower case copy of input string
    static std::string lowerCase(const std::string &in)
    {
        std::string out = in;
        return toLower(out);
    }
};

}

#endif
