#include "lowlevelfile.hpp"

#include <stdexcept>
#include <sstream>
#include <cassert>

#if FILE_API == FILE_API_POSIX
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#endif

#if FILE_API == FILE_API_STDIO
/*
 *
 *  Implementation of LowLevelFile methods using c stdio
 *
 */

LowLevelFile::LowLevelFile ()
{
    mHandle = nullptr;
}

LowLevelFile::~LowLevelFile ()
{
    if (mHandle != nullptr)
        fclose (mHandle);
}

void LowLevelFile::open (char const * filename)
{
    assert (mHandle == nullptr);

    mHandle = fopen (filename, "rb");

    if (mHandle == nullptr)
    {
        std::ostringstream os;
        os << "Failed to open '" << filename << "' for reading.";
        throw std::runtime_error (os.str ());
    }
}

void LowLevelFile::close ()
{
    assert (mHandle != nullptr);

    fclose (mHandle);

    mHandle = nullptr;
}

size_t LowLevelFile::size ()
{
    assert (mHandle != nullptr);

    long oldPosition = ftell (mHandle);

    if (oldPosition == -1)
        throw std::runtime_error ("A query operation on a file failed.");

    if (fseek (mHandle, 0, SEEK_END) != 0)
        throw std::runtime_error ("A query operation on a file failed.");

    long Size = ftell (mHandle);

    if (Size == -1)
        throw std::runtime_error ("A query operation on a file failed.");

    if (fseek (mHandle, oldPosition, SEEK_SET) != 0)
        throw std::runtime_error ("A query operation on a file failed.");

    return size_t (Size);
}

void LowLevelFile::seek (size_t Position)
{
    assert (mHandle != nullptr);

    if (fseek (mHandle, Position, SEEK_SET) != 0)
        throw std::runtime_error ("A seek operation on a file failed.");
}

size_t LowLevelFile::tell ()
{
    assert (mHandle != nullptr);

    long Position = ftell (mHandle);

    if (Position == -1)
        throw std::runtime_error ("A query operation on a file failed.");

    return size_t (Position);
}

size_t LowLevelFile::read (void * data, size_t size)
{
    assert (mHandle != nullptr);

    int amount = fread (data, 1, size, mHandle);

    if (amount == 0 && ferror (mHandle))
        throw std::runtime_error ("A read operation on a file failed.");

    return amount;
}

#elif FILE_API == FILE_API_POSIX
/*
 *
 *  Implementation of LowLevelFile methods using posix IO calls
 *
 */

LowLevelFile::LowLevelFile ()
{
    mHandle = -1;
}

LowLevelFile::~LowLevelFile ()
{
    if (mHandle != -1)
        ::close (mHandle);
}

void LowLevelFile::open (char const * filename)
{
    assert (mHandle == -1);

#ifdef O_BINARY
    static const int openFlags = O_RDONLY | O_BINARY;
#else
    static const int openFlags = O_RDONLY;
#endif

    mHandle = ::open (filename, openFlags, 0);

    if (mHandle == -1)
    {
        std::ostringstream os;
        os << "Failed to open '" << filename << "' for reading: " << strerror(errno);
        throw std::runtime_error (os.str ());
    }
}

void LowLevelFile::close ()
{
    assert (mHandle != -1);

    ::close (mHandle);

    mHandle = -1;
}

size_t LowLevelFile::size ()
{
    assert (mHandle != -1);

    size_t oldPosition = ::lseek (mHandle, 0, SEEK_CUR);

    if (oldPosition == size_t (-1))
    {
        std::ostringstream os;
        os << "An lseek() call failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }

    size_t Size = ::lseek (mHandle, 0, SEEK_END);

    if (Size == size_t (-1))
    {
        std::ostringstream os;
        os << "An lseek() call failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }

    if (lseek (mHandle, oldPosition, SEEK_SET) == -1)
    {
        std::ostringstream os;
        os << "An lseek() call failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }

    return Size;
}

void LowLevelFile::seek (size_t Position)
{
    assert (mHandle != -1);

    if (::lseek (mHandle, Position, SEEK_SET) == -1)
    {
        std::ostringstream os;
        os << "An lseek() call failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }
}

size_t LowLevelFile::tell ()
{
    assert (mHandle != -1);

    size_t Position = ::lseek (mHandle, 0, SEEK_CUR);

    if (Position == size_t (-1))
    {
        std::ostringstream os;
        os << "An lseek() call failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }

    return Position;
}

size_t LowLevelFile::read (void * data, size_t size)
{
    assert (mHandle != -1);

    int amount = ::read (mHandle, data, size);

    if (amount == -1)
    {
        std::ostringstream os;
        os << "An attempt to read " << size << "bytes failed:" << strerror(errno);
        throw std::runtime_error (os.str ());
    }

    return amount;
}

#elif FILE_API == FILE_API_WIN32

#include <boost/locale.hpp>
/*
 *
 *  Implementation of LowLevelFile methods using Win32 API calls
 *
 */

LowLevelFile::LowLevelFile ()
{
    mHandle = INVALID_HANDLE_VALUE;
}

LowLevelFile::~LowLevelFile ()
{
    if (mHandle != INVALID_HANDLE_VALUE)
        CloseHandle (mHandle);
}

void LowLevelFile::open (char const * filename)
{
    assert (mHandle == INVALID_HANDLE_VALUE);

    std::wstring wname = boost::locale::conv::utf_to_utf<wchar_t>(filename);
    HANDLE handle = CreateFileW (wname.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (handle == INVALID_HANDLE_VALUE)
    {
        std::ostringstream os;
        os << "Failed to open '" << filename << "' for reading: " << GetLastError();
        throw std::runtime_error (os.str ());
    }

    mHandle = handle;
}

void LowLevelFile::close ()
{
    assert (mHandle != INVALID_HANDLE_VALUE);

    CloseHandle (mHandle);

    mHandle = INVALID_HANDLE_VALUE;
}

size_t LowLevelFile::size ()
{
    assert (mHandle != INVALID_HANDLE_VALUE);

    BY_HANDLE_FILE_INFORMATION info;

    if (!GetFileInformationByHandle (mHandle, &info))
        throw std::runtime_error ("A query operation on a file failed.");

    if (info.nFileSizeHigh != 0)
        throw std::runtime_error ("Files greater that 4GB are not supported.");

    return info.nFileSizeLow;
}

void LowLevelFile::seek (size_t Position)
{
    assert (mHandle != INVALID_HANDLE_VALUE);

    if (SetFilePointer (mHandle, Position, nullptr, SEEK_SET) == INVALID_SET_FILE_POINTER)
        if (GetLastError () != NO_ERROR)
            throw std::runtime_error ("A seek operation on a file failed.");
}

size_t LowLevelFile::tell ()
{
    assert (mHandle != INVALID_HANDLE_VALUE);

    DWORD value = SetFilePointer (mHandle, 0, nullptr, SEEK_CUR);

    if (value == INVALID_SET_FILE_POINTER && GetLastError () != NO_ERROR)
        throw std::runtime_error ("A query operation on a file failed.");

    return value;
}

size_t LowLevelFile::read (void * data, size_t size)
{
    assert (mHandle != INVALID_HANDLE_VALUE);

    DWORD read;

    if (!ReadFile (mHandle, data, size, &read, nullptr))
        throw std::runtime_error ("A read operation on a file failed.");

    return read;
}

#endif
