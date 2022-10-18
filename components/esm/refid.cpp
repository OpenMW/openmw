#include "refid.hpp"

#include <iostream>

#include "components/misc/strings/algorithm.hpp"

namespace ESM
{
    bool RefId::operator==(const RefId& rhs) const
    {
        return this->mId == rhs.mId;
    }

    bool RefId::operator<(const RefId& rhs) const
    {
        return mId < rhs.mId;
    }

    bool RefId::operator>(const RefId& rhs) const
    {
        return mId > rhs.mId;
    }

    std::ostream& operator<<(std::ostream& os, const RefId& refId)
    {
        os << refId.getRefIdString();
        return os;
    }

    RefId RefId::stringRefId(std::string_view id)
    {
        RefId newRefId;
        newRefId.mId = Misc::StringUtils::lowerCase(id);
        return newRefId;
    }

    bool RefId::operator==(std::string_view rhs) const
    {
        return Misc::StringUtils::ciEqual(mId, rhs);
    }

    const RefId RefId::sEmpty = {};
}
