#include "indexrefid.hpp"

#include <ostream>
#include <sstream>

#include "esmcommon.hpp"

namespace ESM
{
    std::string IndexRefId::toString() const
    {
        return ESM::NAME(mRecordType).toString() + ", " + std::to_string(mValue);
    }

    std::string IndexRefId::toDebugString() const
    {
        std::ostringstream stream;
        stream << *this;
        return stream.str();
    }

    std::ostream& operator<<(std::ostream& stream, IndexRefId value)
    {
        return stream << "Index{" << ESM::NAME(value.mRecordType).toStringView() << ", " << value.mValue << '}';
    }
}
