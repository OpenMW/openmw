#include "file.hpp"

#include <cassert>
#include <components/misc/windows.hpp>
#include <stdexcept>
#include <string>

#include <components/files/conversion.hpp>

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
            throw std::runtime_error(std::string("Failed to open '") + Files::pathToUnicodeString(filename)
                + "' for reading: " + std::to_string(GetLastError()));
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
        {
            if (auto errCode = GetLastError(); errCode != ERROR_SUCCESS)
            {
                throw std::runtime_error(std::string("An fseek() call failed: ") + std::to_string(errCode));
            }
        }
    }

    size_t size(Handle handle)
    {
        auto nativeHandle = getNativeHandle(handle);

        BY_HANDLE_FILE_INFORMATION info;

        if (!GetFileInformationByHandle(nativeHandle, &info))
            throw std::runtime_error("A query operation on a file failed.");

        if (info.nFileSizeHigh != 0)
            throw std::runtime_error("Files greater that 4GB are not supported.");

        return info.nFileSizeLow;
    }

    size_t tell(Handle handle)
    {
        auto nativeHandle = getNativeHandle(handle);

        DWORD value = SetFilePointer(nativeHandle, 0, nullptr, SEEK_CUR);
        if (value == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
            throw std::runtime_error("A query operation on a file failed.");

        return value;
    }

    size_t read(Handle handle, void* data, size_t size)
    {
        auto nativeHandle = getNativeHandle(handle);

        DWORD bytesRead{};

        if (!ReadFile(nativeHandle, data, static_cast<DWORD>(size), &bytesRead, nullptr))
            throw std::runtime_error(
                std::string("A read operation on a file failed: ") + std::to_string(GetLastError()));

        return bytesRead;
    }
}
