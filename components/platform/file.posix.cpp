#include "file.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <string>
#include <cassert>

namespace Platform::File {

    static auto getNativeHandle(Handle handle)
    {
        assert(handle != Handle::Invalid);

        return static_cast<int>(handle);
    }

    static int getNativeSeekType(SeekType seek)
    {
        if (seek == SeekType::Begin)
            return SEEK_SET;
        if (seek == SeekType::Current)
            return SEEK_CUR;
        if (seek == SeekType::End)
            return SEEK_END;
        return -1;
    }

    Handle open(const char* filename)
    {
#ifdef O_BINARY
        static const int openFlags = O_RDONLY | O_BINARY;
#else
        static const int openFlags = O_RDONLY;
#endif

        auto handle = ::open(filename, openFlags, 0);
        if (handle == -1)
        {
            throw std::runtime_error(std::string("Failed to open '") + filename + "' for reading: " + strerror(errno));
        }
        return static_cast<Handle>(handle);
    }

    void close(Handle handle)
    {
        auto nativeHandle = getNativeHandle(handle);

        ::close(nativeHandle);
    }

    void seek(Handle handle, size_t position, SeekType type /*= SeekType::Begin*/)
    {
        const auto nativeHandle = getNativeHandle(handle);
        const auto nativeSeekType = getNativeSeekType(type);

        if (::lseek(nativeHandle, position, nativeSeekType) == -1)
        {
            throw std::runtime_error("An lseek() call failed: " + std::string(strerror(errno)));
        }
    }

    size_t size(Handle handle)
    {
        const auto oldPos = tell(handle);

        seek(handle, 0, SeekType::End);
        const auto fileSize = tell(handle);
        seek(handle, oldPos, SeekType::Begin);


        return static_cast<size_t>(fileSize);
    }

    size_t tell(Handle handle)
    {
        auto nativeHandle = getNativeHandle(handle);

        size_t position = ::lseek(nativeHandle, 0, SEEK_CUR);
        if (position == size_t(-1))
        {
            throw std::runtime_error("An lseek() call failed: " + std::string(strerror(errno)));
        }
        return position;
    }

    size_t read(Handle handle, void* data, size_t size)
    {
        auto nativeHandle = getNativeHandle(handle);

        int amount = ::read(nativeHandle, data, size);
        if (amount == -1)
        {
            throw std::runtime_error("An attempt to read " + std::to_string(size) + " bytes failed: " + strerror(errno));
        }
        return amount;
    }

}
