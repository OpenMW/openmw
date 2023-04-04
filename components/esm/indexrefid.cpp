#include "indexrefid.hpp"

#include "serializerefid.hpp"

#include <ostream>

namespace ESM
{
    std::string IndexRefId::toString() const
    {
        std::string result;
        result.resize(sizeof(mRecordType) + getIntegralSize(mValue) + 3, '\0');
        std::memcpy(result.data(), &mRecordType, sizeof(mRecordType));
        result[sizeof(mRecordType)] = ':';
        serializeIntegral(mValue, sizeof(mRecordType) + 1, result);
        return result;
    }

    std::string IndexRefId::toDebugString() const
    {
        std::string result;
        serializeRefIdPrefix(sizeof(mRecordType) + getIntegralSize(mValue) + 1, indexRefIdPrefix, result);
        std::memcpy(result.data() + indexRefIdPrefix.size(), &mRecordType, sizeof(mRecordType));
        result[indexRefIdPrefix.size() + sizeof(mRecordType)] = ':';
        serializeIntegral(mValue, indexRefIdPrefix.size() + sizeof(mRecordType) + 1, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, IndexRefId value)
    {
        return stream << value.toDebugString();
    }
}
