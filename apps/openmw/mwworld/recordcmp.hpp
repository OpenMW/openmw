#ifndef OPENMW_MWWORLD_RECORDCMP_H
#define OPENMW_MWWORLD_RECORDCMP_H

#include <components/esm/records.hpp>

#include <components/misc/stringops.hpp>

namespace MWWorld
{
    struct RecordCmp
    {
        template <class T>
        bool operator()(const T &x, const T& y) const {
            return x.mId < y.mId;
        }
    };

    template <>
    inline bool RecordCmp::operator()<ESM::Dialogue>(const ESM::Dialogue &x, const ESM::Dialogue &y) const {
        return Misc::StringUtils::ciLess(x.mId, y.mId);
    }

    template <>
    inline bool RecordCmp::operator()<ESM::Cell>(const ESM::Cell &x, const ESM::Cell &y) const {
        return Misc::StringUtils::ciLess(x.mName, y.mName);
    }

    template <>
    inline bool RecordCmp::operator()<ESM::Pathgrid>(const ESM::Pathgrid &x, const ESM::Pathgrid &y) const {
        return Misc::StringUtils::ciLess(x.mCell, y.mCell);
    }

} // end namespace
#endif
