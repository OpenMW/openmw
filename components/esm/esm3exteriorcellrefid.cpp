#include "esm3exteriorcellrefid.hpp"
#include "serializerefid.hpp"

#include <limits>
#include <ostream>
#include <sstream>

namespace ESM
{
    std::string ESM3ExteriorCellRefId::toString() const
    {
        constexpr std::size_t separator = 1;
        std::string result;
        result.resize(separator + getDecIntegralCapacity(mX) + separator + getDecIntegralCapacity(mY), '\0');
        result[0] = '#';
        const std::size_t endX = serializeDecIntegral(mX, separator, result);
        result[endX] = ' ';
        const std::size_t endY = serializeDecIntegral(mY, endX + separator, result);
        result.resize(endY);
        return result;
    }

    std::string ESM3ExteriorCellRefId::toDebugString() const
    {
        constexpr std::size_t separator = 1;
        std::string result;
        serializeRefIdPrefix(
            getDecIntegralCapacity(mX) + separator + getDecIntegralCapacity(mY), esm3ExteriorCellRefIdPrefix, result);
        const std::size_t endX = serializeDecIntegral(mX, esm3ExteriorCellRefIdPrefix.size(), result);
        result[endX] = ':';
        const std::size_t endY = serializeDecIntegral(mY, endX + separator, result);
        result.resize(endY);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, ESM3ExteriorCellRefId value)
    {
        return stream << value.toDebugString();
    }
}
