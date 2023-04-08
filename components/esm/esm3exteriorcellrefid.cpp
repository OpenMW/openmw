#include "esm3exteriorcellrefid.hpp"
#include "serializerefid.hpp"

#include <limits>
#include <ostream>
#include <sstream>

namespace ESM
{
    std::string ESM3ExteriorCellRefId::toString() const
    {
        std::string result;
        result.resize(getDecIntegralCapacity(mX) + getDecIntegralCapacity(mY) + 3, '\0');
        const std::size_t endX = serializeDecIntegral(mX, 0, result);
        result[endX] = ':';
        const std::size_t endY = serializeDecIntegral(mY, endX + 1, result);
        result.resize(endY);
        return result;
    }

    std::string ESM3ExteriorCellRefId::toDebugString() const
    {
        std::string result;
        serializeRefIdPrefix(
            getDecIntegralCapacity(mX) + getDecIntegralCapacity(mY) + 1, esm3ExteriorCellRefIdPrefix, result);
        const std::size_t endX = serializeDecIntegral(mX, esm3ExteriorCellRefIdPrefix.size(), result);
        result[endX] = ':';
        const std::size_t endY = serializeDecIntegral(mY, endX + 1, result);
        result.resize(endY);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, ESM3ExteriorCellRefId value)
    {
        return stream << value.toDebugString();
    }
}
