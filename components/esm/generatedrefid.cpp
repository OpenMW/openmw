#include "generatedrefid.hpp"

#include "serializerefid.hpp"

#include <ostream>

namespace ESM
{
    std::string GeneratedRefId::toString() const
    {
        std::string result;
        result.resize(getIntegralSize(mValue) + 2, '\0');
        serializeIntegral(mValue, 0, result);
        return result;
    }

    std::string GeneratedRefId::toDebugString() const
    {
        std::string result;
        serializeRefIdValue(mValue, generatedRefIdPrefix, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, GeneratedRefId value)
    {
        return stream << value.toDebugString();
    }
}
