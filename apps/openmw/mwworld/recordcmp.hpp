#ifndef OPENMW_MWWORLD_RECORDCMP_H
#define OPENMW_MWWORLD_RECORDCMP_H

#include <string>
#include <cctype>
#include <algorithm>

#include <components/esm/records.hpp>

namespace MWWorld
{
    /// \todo move this to another location
    class StringUtils
    {
        struct ci
        {
            bool operator()(int x, int y) const {
                return std::tolower(x) < std::tolower(y);
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
                if (std::tolower(*xit) != std::tolower(*yit)) {
                    return false;
                }
            }
            return true;
        }

        /// Transforms input string to lower case w/o copy
        static std::string &toLower(std::string &inout) {
            std::transform(
                inout.begin(),
                inout.end(),
                inout.begin(),
                (int (*)(int)) std::tolower
            );
            return inout;
        }

        /// Returns lower case copy of input string
        static std::string lowerCase(const std::string &in)
        {
            std::string out = in;
            return toLower(out);
        }
    };

    struct RecordCmp
    {
        template <class T>
        bool operator()(const T &x, const T& y) const {
            return x.mId < y.mId;
        }
    };

    template <>
    inline bool RecordCmp::operator()<ESM::Dialogue>(const ESM::Dialogue &x, const ESM::Dialogue &y) const {
        return StringUtils::ciLess(x.mId, y.mId);
    }

    template <>
    inline bool RecordCmp::operator()<ESM::Cell>(const ESM::Cell &x, const ESM::Cell &y) const {
        return StringUtils::ciLess(x.mName, y.mName);
    }

    template <>
    inline bool RecordCmp::operator()<ESM::Pathgrid>(const ESM::Pathgrid &x, const ESM::Pathgrid &y) const {
        return StringUtils::ciLess(x.mCell, y.mCell);
    }

} // end namespace
#endif
