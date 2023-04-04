#include "esm3exteriorcellrefid.hpp"
#include "serializerefid.hpp"

#include <ostream>
#include <sstream>

namespace ESM
{
    std::string ESM3ExteriorCellRefId::toString() const
    {
        std::string result;
        std::size_t integralSizeX = getIntegralSize(mX);
        result.resize(integralSizeX + getIntegralSize(mY) + 3, '\0');
        serializeIntegral(mX, 0, result);
        result[integralSizeX] = ':';
        serializeIntegral(mY, integralSizeX + 1, result);
        return result;
    }

    std::string ESM3ExteriorCellRefId::toDebugString() const
    {
        std::string result;
        std::size_t integralSizeX = getIntegralSize(mX);

        serializeRefIdPrefix(integralSizeX + getIntegralSize(mY) + 1, esm3ExteriorCellRefIdPrefix, result);
        serializeIntegral(mX, esm3ExteriorCellRefIdPrefix.size(), result);
        result[esm3ExteriorCellRefIdPrefix.size() + integralSizeX] = ':';
        serializeIntegral(mY, esm3ExteriorCellRefIdPrefix.size() + integralSizeX + 1, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, ESM3ExteriorCellRefId value)
    {
        return stream << "Vec2i{" << value.mX << "," << value.mY << '}';
    }
}
