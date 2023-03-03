#include "refid.hpp"

#include <iostream>

#include "components/misc/strings/algorithm.hpp"

namespace ESM
{
    bool RefId::operator==(const RefId& rhs) const
    {
        return Misc::StringUtils::ciEqual(mId, rhs.mId);
    }

    bool RefId::operator<(const RefId& rhs) const
    {
        return Misc::StringUtils::ciLess(mId, rhs.mId);
    }

    bool operator<(const RefId& lhs, std::string_view rhs)
    {
        return Misc::StringUtils::ciLess(lhs.mId, rhs);
    }

    bool operator<(std::string_view lhs, const RefId& rhs)
    {
        return Misc::StringUtils::ciLess(lhs, rhs.mId);
    }

    std::ostream& operator<<(std::ostream& os, const RefId& refId)
    {
        os << refId.getRefIdString();
        return os;
    }

    RefId RefId::stringRefId(std::string_view id)
    {
        RefId newRefId;
        newRefId.mId = id;
        return newRefId;
    }

    RefId RefId::formIdRefId(const ESM4::FormId id)
    {
        return ESM::RefId::stringRefId(ESM4::formIdToString(id));
    }

    bool RefId::operator==(std::string_view rhs) const
    {
        return Misc::StringUtils::ciEqual(mId, rhs);
    }

    bool RefId::startsWith(std::string_view prefix) const
    {
        return Misc::StringUtils::ciStartsWith(mId, prefix);
    }

    bool RefId::contains(std::string_view subString) const
    {
        return Misc::StringUtils::ciFind(mId, subString) != std::string_view::npos;
    }

    const RefId RefId::sEmpty = {};
}

std::size_t std::hash<ESM::RefId>::operator()(const ESM::RefId& k) const
{
    return Misc::StringUtils::CiHash()(k.getRefIdString());
}
