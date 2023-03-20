#include "formidrefid.hpp"

#include <ostream>
#include <sstream>

namespace ESM
{
    std::string FormIdRefId::toString() const
    {
        return std::to_string(mValue);
    }

    std::string FormIdRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, FormIdRefId value)
    {
        return stream << "FormId{" << value.mValue << '}';
    }
}
