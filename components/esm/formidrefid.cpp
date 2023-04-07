#include "formidrefid.hpp"

#include "serializerefid.hpp"

#include <ostream>

namespace ESM
{
    std::string FormIdRefId::toString() const
    {
        std::string result;
        result.resize(getHexIntegralSize(mValue) + 2, '\0');
        serializeHexIntegral(mValue, 0, result);
        return result;
    }

    std::string FormIdRefId::toDebugString() const
    {
        std::string result;
        serializeRefIdValue(mValue, formIdRefIdPrefix, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, FormIdRefId value)
    {
        return stream << value.toDebugString();
    }
}
