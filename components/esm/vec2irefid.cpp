#include "vec2irefid.hpp"

#include <ostream>
#include <sstream>

namespace ESM
{
    std::string Vec2iRefId::toString() const
    {
        std::ostringstream stream;
        stream << "# " << mValue.first << ", " << mValue.second;
        return stream.str();
    }

    std::string Vec2iRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, Vec2iRefId value)
    {
        return stream << "Vec2i{" << value.mValue.first << "," << value.mValue.second << '}';
    }
}
