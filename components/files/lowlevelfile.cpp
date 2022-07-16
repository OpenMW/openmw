#include "lowlevelfile.hpp"

#include <stdexcept>
#include <sstream>
#include <cassert>

namespace File = Platform::File;

LowLevelFile::~LowLevelFile()
{
    if (mHandle != File::Handle::Invalid)
        File::close(mHandle);
}

void LowLevelFile::open(char const* filename)
{
    mHandle = File::open(filename);
}

void LowLevelFile::close()
{
    if (mHandle != File::Handle::Invalid)
        File::close(mHandle);
    mHandle = File::Handle::Invalid;
}

size_t LowLevelFile::size()
{
    assert(mHandle != File::Handle::Invalid);

    return File::size(mHandle);
}

void LowLevelFile::seek(size_t position)
{
    assert(mHandle != File::Handle::Invalid);

    return File::seek(mHandle, position);
}

size_t LowLevelFile::tell()
{
    assert(mHandle != File::Handle::Invalid);

    return File::tell(mHandle);
}

size_t LowLevelFile::read(void* data, size_t size)
{
    assert(mHandle != File::Handle::Invalid);

    return File::read(mHandle, data, size);
}
