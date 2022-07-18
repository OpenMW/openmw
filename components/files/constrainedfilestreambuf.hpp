#ifndef OPENMW_CONSTRAINEDFILESTREAMBUF_H
#define OPENMW_CONSTRAINEDFILESTREAMBUF_H

#include <components/platform/file.hpp>

#include <streambuf>

namespace Files
{
    /// A file streambuf constrained to a specific region in the file, specified by the 'start' and 'length' parameters.
    class ConstrainedFileStreamBuf final : public std::streambuf
    {
    public:
        ConstrainedFileStreamBuf(const std::string& fname, std::size_t start, std::size_t length);

        int_type underflow() final;

        pos_type seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode) final;

        pos_type seekpos(pos_type pos, std::ios_base::openmode mode) final;

    private:
        std::size_t mOrigin;
        std::size_t mSize;
        Platform::File::ScopedHandle mFile;
        char mBuffer[8192]{ 0 };
    };
}

#endif
