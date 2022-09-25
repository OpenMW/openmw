#include "refid.hpp"

#include <iostream>

#include "components/misc/strings/algorithm.hpp"

bool ESM::RefId::ciEqual(const RefId & left, const RefId & right)
{
    return Misc::StringUtils::ciEqual(left.mId, right.mId);
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

//template<>
//std::size_t std::hash<ESM::RefId>::operator()(const ESM::RefId& k) const
//{
//    using std::size_t;
//    using std::hash;
//    using std::string;
//
//    // Compute individual hash values for first,
//    // second and third and combine them using XOR
//    // and bit shifting:
//
//    return hash<string>()(k.getRefIdString());
//}
