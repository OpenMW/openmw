#include "formidrefid.hpp"

#include <cassert>
#include <ostream>

#include "serializerefid.hpp"

namespace ESM
{
    std::string FormIdRefId::toString() const
    {
        std::string result;
        assert((mValue.mIndex & 0xff000000) == 0);
        size_t v = (static_cast<size_t>(mValue.mContentFile) << 24) | mValue.mIndex;
        result.resize(getHexIntegralSize(v) + 2, '\0');
        serializeHexIntegral(v, 0, result);
        return result;
    }

    std::string FormIdRefId::toDebugString() const
    {
        std::string result;
        assert((mValue.mIndex & 0xff000000) == 0);
        size_t v = (static_cast<size_t>(mValue.mContentFile) << 24) | mValue.mIndex;
        serializeRefIdValue(v, formIdRefIdPrefix, result);
        return result;
    }

    std::ostream& operator<<(std::ostream& stream, FormIdRefId value)
    {
        return stream << value.toDebugString();
    }
}
