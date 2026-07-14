#ifndef OPENMW_COMPONENTS_FILES_MEMORYSTREAM_H
#define OPENMW_COMPONENTS_FILES_MEMORYSTREAM_H

#include <istream>

namespace Files
{

    struct MemBuf : std::streambuf
    {
        MemBuf(char const* buffer, size_t size)
            // a streambuf isn't specific to istreams, so we need a non-const pointer :/
            : mBufferStart(const_cast<char*>(buffer))
            , mBufferEnd(mBufferStart + size)
        {
            this->setg(mBufferStart, mBufferStart, mBufferEnd);
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
        {
            if (dir == std::ios_base::cur)
                setg(mBufferStart, gptr() + off, mBufferEnd);
            else
                setg(mBufferStart, (dir == std::ios_base::beg ? mBufferStart : mBufferEnd) + off, mBufferEnd);

            return gptr() - mBufferStart;
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
        {
            return seekoff(pos, std::ios_base::beg, which);
        }

    protected:
        char* mBufferStart;
        char* mBufferEnd;
    };

    /// @brief A variant of std::istream that reads from a constant in-memory buffer.
    struct IMemStream : virtual MemBuf, std::istream
    {
        IMemStream(char const* buffer, size_t size)
            : MemBuf(buffer, size)
            , std::istream(static_cast<std::streambuf*>(this))
        {
        }
    };

}

#endif
