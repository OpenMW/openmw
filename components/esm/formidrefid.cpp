#include "formidrefid.hpp"

#include <ostream>

#include "serializerefid.hpp"

namespace ESM
{
    namespace
    {
        std::uint64_t truncate(FormId value)
        {
            return (static_cast<std::uint64_t>(value.mContentFile) << 24) | value.mIndex;
        }
    }

    std::string FormIdRefId::toString() const
    {
        std::string result;
        const std::uint64_t v = truncate(mValue);
        result.resize(getHexIntegralSizeWith0x(v), '\0');
        serializeHexIntegral(v, 0, result);
        return result;
    }

    std::string FormIdRefId::toDebugString() const
    {
        std::string result;
        const std::uint64_t v = truncate(mValue);
        serializeRefIdValue(v, formIdRefIdPrefix, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, FormIdRefId value)
    {
        return stream << value.toDebugString();
    }
}
