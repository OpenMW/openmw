// SPDX-License-Identifier: GPL-3.0-only
#ifndef OPENMW_COMPONENTS_SPT_STREAM_HPP
#define OPENMW_COMPONENTS_SPT_STREAM_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

#include <components/files/binarystream.hpp>

namespace Spt
{
    struct SPTEOFError : std::runtime_error
    {
        SPTEOFError();
    };

    class SPTStream : public Files::BinaryStream
    {
    public:
        using Files::BinaryStream::BinaryStream;

        int32_t readSection();
        std::string readIntString() { return readSizedString(4096); }
    };
}

#endif
