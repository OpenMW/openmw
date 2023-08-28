#include "formid.hpp"

#include <charconv>
#include <cstring>

std::string ESM::FormId::toString(std::string_view prefix) const
{
    std::string res;
    res.resize(prefix.length() + 20);
    std::memcpy(res.data(), prefix.data(), prefix.size());
    char* buf = res.data() + prefix.size();
    uint64_t value;
    if (hasContentFile())
    {
        if ((mIndex & 0xff000000) != 0)
            throw std::invalid_argument("Invalid FormId index value: " + std::to_string(mIndex));
        value = mIndex | (static_cast<uint64_t>(mContentFile) << 24);
    }
    else
    {
        *(buf++) = '@';
        value = mIndex | (static_cast<uint64_t>(-mContentFile - 1) << 32);
    }
    *(buf++) = '0';
    *(buf++) = 'x';
    const auto r = std::to_chars(buf, res.data() + res.size(), value, 16);
    if (r.ec != std::errc())
        throw std::system_error(std::make_error_code(r.ec), "ESM::FormId::toString failed");
    res.resize(r.ptr - res.data());
    return res;
}

uint32_t ESM::FormId::toUint32() const
{
    if (isSet() && !hasContentFile())
        throw std::runtime_error("Generated FormId can not be converted to 32bit format");
    if (mContentFile > 0xfe)
        throw std::runtime_error("FormId with mContentFile > 0xFE can not be converted to 32bit format");
    return (mIndex & 0xffffff) | ((hasContentFile() ? mContentFile : 0xff) << 24);
}
