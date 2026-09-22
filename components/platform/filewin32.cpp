#include "file.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>

#include <components/files/conversion.hpp>
#include <components/misc/windows.hpp>

namespace Platform::File
{

    static auto getNativeHandle(Handle handle)
    {
        assert(handle != Handle::Invalid);

        return reinterpret_cast<HANDLE>(static_cast<intptr_t>(handle));
    }

    static int getNativeSeekType(SeekType seek)
    {
        if (seek == SeekType::Begin)
            return FILE_BEGIN;
        if (seek == SeekType::Current)
            return FILE_CURRENT;
        if (seek == SeekType::End)
            return FILE_END;
        return -1;
    }

    Handle open(const std::filesystem::path& filename)
    {
        HANDLE handle = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        if (handle == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error(std::format(
                "Failed to open '{}' for reading: {}", Files::pathToUnicodeString(filename), GetLastError()));
        }
        return static_cast<Handle>(reinterpret_cast<intptr_t>(handle));
    }

    void close(Handle handle)
    {
        auto nativeHandle = getNativeHandle(handle);
        CloseHandle(nativeHandle);
    }

    void seek(Handle handle, size_t position, SeekType type /*= SeekType::Begin*/)
    {
        const auto nativeHandle = getNativeHandle(handle);
        const auto nativeSeekType = getNativeSeekType(type);

        LARGE_INTEGER li;
        li.QuadPart = static_cast<LONGLONG>(position);
        if (!SetFilePointerEx(nativeHandle, li, nullptr, nativeSeekType))
            throw std::runtime_error(std::format("Failed to seek in file: {}", GetLastError()));
    }

    size_t size(Handle handle)
    {
        const auto nativeHandle = getNativeHandle(handle);
        LARGE_INTEGER li;
        if (!GetFileSizeEx(nativeHandle, &li))
            throw std::runtime_error(std::format("Failed to get file size: {}", GetLastError()));

        return static_cast<size_t>(li.QuadPart);
    }

    size_t tell(Handle handle)
    {
        const auto nativeHandle = getNativeHandle(handle);
        LARGE_INTEGER distance;
        distance.QuadPart = 0;
        LARGE_INTEGER li;
        if (!SetFilePointerEx(nativeHandle, distance, &li, FILE_CURRENT))
            throw std::runtime_error(std::format("Failed to get file offset: {}", GetLastError()));

        return static_cast<size_t>(li.QuadPart);
    }

    size_t read(Handle handle, void* data, size_t size)
    {
        const auto nativeHandle = getNativeHandle(handle);

        DWORD bytesRead{};
        size = std::min<size_t>(size, std::numeric_limits<DWORD>::max());
        if (!ReadFile(nativeHandle, data, static_cast<DWORD>(size), &bytesRead, nullptr))
            throw std::runtime_error(std::format("A read operation on a file failed: {}", GetLastError()));

        return bytesRead;
    }
}
