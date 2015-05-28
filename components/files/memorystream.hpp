#ifndef OPENMW_COMPONENTS_FILES_MEMORYSTREAM_H
#define OPENMW_COMPONENTS_FILES_MEMORYSTREAM_H

#include <istream>

namespace Files
{

    struct MemBuf : std::streambuf
    {
        MemBuf(char const* buffer, size_t size)
        {
            // a streambuf isn't specific to istreams, so we need a non-const pointer :/
            char* nonconstBuffer = (const_cast<char*>(buffer));
            this->setg(nonconstBuffer, nonconstBuffer, nonconstBuffer + size);
        }
    };

    /// @brief A variant of std::istream that reads from a constant in-memory buffer.
    struct IMemStream: virtual MemBuf, std::istream
    {
        IMemStream(char const* buffer, size_t size)
            : MemBuf(buffer, size)
            , std::istream(static_cast<std::streambuf*>(this))
        {
        }
    };

}

#endif
