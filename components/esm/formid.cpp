#include "formid.hpp"

std::string ESM::FormId::toString() const
{
    return std::to_string(mIndex) + "_" + std::to_string(mContentFile);
}

uint32_t ESM::FormId::toUint32() const
{
    if (isSet() && !hasContentFile())
        throw std::runtime_error("Generated FormId can not be converted to 32bit format");
    if (mContentFile > 0xfe)
        throw std::runtime_error("FormId with mContentFile > 0xFE can not be converted to 32bit format");
    return (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff) << 24);
}
