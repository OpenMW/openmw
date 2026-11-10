#ifndef OPENMW_COMPONENTS_BGSM_STREAM_HPP
#define OPENMW_COMPONENTS_BGSM_STREAM_HPP

#include <string>

#include <components/files/binarystream.hpp>

namespace Bgsm
{
    class BGSMStream : public Files::BinaryStream
    {
    public:
        using Files::BinaryStream::BinaryStream;
        using Files::BinaryStream::read;

        void read(std::string& str);
    };
}

#endif
