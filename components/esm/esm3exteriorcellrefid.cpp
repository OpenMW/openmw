#include "esm3exteriorcellrefid.hpp"

#include <ostream>
#include <sstream>

namespace ESM
{
    std::string ESM3ExteriorCellRefId::toString() const
    {
        std::ostringstream stream;
        stream << "# " << mY << ", " << mY;
        return stream.str();
    }

    std::string ESM3ExteriorCellRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, ESM3ExteriorCellRefId value)
    {
        return stream << "Vec2i{" << value.mX << "," << value.mY << '}';
    }
}
