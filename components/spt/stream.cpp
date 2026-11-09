// SPDX-License-Identifier: GPL-3.0-only
#include "stream.hpp"

#include <cstdint>
#include <istream>
#include <stdexcept>
#include <string>

namespace Spt
{
    SPTEOFError::SPTEOFError()
        : std::runtime_error("SPT stream EOF")
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
}
