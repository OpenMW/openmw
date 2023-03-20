#include "generatedrefid.hpp"

#include <ostream>
#include <sstream>

namespace ESM
{
    std::string GeneratedRefId::toString() const
    {
        return std::to_string(mValue);
    }

    std::string GeneratedRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, GeneratedRefId value)
    {
        return stream << "Generated{" << value.mValue << '}';
    }
}
