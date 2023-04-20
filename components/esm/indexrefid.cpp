#include "indexrefid.hpp"

#include "serializerefid.hpp"

#include <ostream>

namespace ESM
{
    std::string IndexRefId::toString() const
    {
        std::string result;
        constexpr std::size_t separator = 1;
        result.resize(sizeof(mRecordType) + separator + getHexIntegralSizeWith0x(mValue), '\0');
        std::memcpy(result.data(), &mRecordType, sizeof(mRecordType));
        result[sizeof(mRecordType)] = ':';
        serializeHexIntegral(mValue, sizeof(mRecordType) + separator, result);
        return result;
    }

    std::string IndexRefId::toDebugString() const
    {
        std::string result;
        constexpr std::size_t separator = 1;
        serializeRefIdPrefix(
            sizeof(mRecordType) + separator + getHexIntegralSizeWith0x(mValue), indexRefIdPrefix, result);
        std::memcpy(result.data() + indexRefIdPrefix.size(), &mRecordType, sizeof(mRecordType));
        result[indexRefIdPrefix.size() + sizeof(mRecordType)] = ':';
        serializeHexIntegral(mValue, indexRefIdPrefix.size() + sizeof(mRecordType) + separator, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, IndexRefId value)
    {
        return stream << value.toDebugString();
    }
}
