#include "refid.hpp"

#include <iostream>

#include "components/misc/strings/algorithm.hpp"

bool ESM::RefId::operator==(const RefId& rhs) const
{
    return Misc::StringUtils::ciEqual(this->mId, rhs.mId);
}

namespace ESM
{
    std::ostream& operator<<(std::ostream& os, const ESM::RefId& refId)
    {
        os << refId.getRefIdString();
        return os;
    }
}

ESM::RefId ESM::RefId::stringRefId(const std::string_view & id)
{
    RefId newRefId;
    newRefId.mId = Misc::StringUtils::lowerCase(id);
    return newRefId;
}

const ESM::RefId ESM::RefId::sEmpty = ESM::RefId::stringRefId("");

