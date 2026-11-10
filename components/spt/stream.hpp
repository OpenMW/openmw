#ifndef OPENMW_COMPONENTS_SPT_STREAM_HPP
#define OPENMW_COMPONENTS_SPT_STREAM_HPP

#include <cstdint>
#include <istream>
#include <string>

#include <components/files/binarystream.hpp>

namespace Spt
{
    class SPTStream : public Files::BinaryStream
    {
    public:
        using Files::BinaryStream::BinaryStream;

        bool eof() { return mStream->peek() == std::char_traits<char>::eof(); }
        int32_t readSection();
        std::string readIntString() { return readSizedString(4096); }
    };
}

#endif
