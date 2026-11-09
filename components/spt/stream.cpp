// SPDX-License-Identifier: GPL-3.0-only
#include "stream.hpp"

#include <cstdint>
#include <format>
#include <istream>
#include <stdexcept>
#include <string>

namespace Spt
{
    SPTEOFError::SPTEOFError()
        : std::runtime_error("SPT stream EOF")
    {
    }

    SPTStream::SPTStream(Files::IStreamPtr&& stream)
        : mStream(std::move(stream))
    {
    }

    int32_t SPTStream::readSection()
    {
        if (mStream->peek() == std::char_traits<char>::eof())
            throw SPTEOFError();
        int32_t value;
        read(value);
        return value;
    }

    std::string SPTStream::readIntString()
    {
        uint32_t length;
        read(length);
        if (length > 4096)
            throw std::runtime_error("SPT intstring too long: " + std::to_string(length));
        std::string str(length, '\0');
        mStream->read(str.data(), length);
        if (mStream->fail())
            throw std::runtime_error(std::format(
                "Failed to read intstring of {} chars: {}", length, std::generic_category().message(errno)));
        return str;
    }
}
