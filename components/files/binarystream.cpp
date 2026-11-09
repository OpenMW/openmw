// SPDX-License-Identifier: GPL-3.0-only
#include "binarystream.hpp"

namespace Files
{
    std::string BinaryStream::readSizedString(std::uint32_t maxLength)
    {
        std::uint32_t length;
        read(length);
        // Corrupt data can claim any length, so don't try to allocate it
        if (length > maxLength)
            throw std::runtime_error(std::format("String length {} is over the limit of {}", length, maxLength));
        std::string str(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->fail())
            throw std::runtime_error(std::format(
                "Failed to read sized string of {} chars: {}", length, std::generic_category().message(errno)));
        return str;
    }
}
