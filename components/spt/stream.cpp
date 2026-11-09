// SPDX-License-Identifier: GPL-3.0-only
#include "stream.hpp"

#include <cstdint>

namespace Spt
{
    int32_t SPTStream::readSection()
    {
        int32_t value;
        read(value);
        return value;
    }
}
