#include "constrainedfilestreambuf.hpp"

#include <algorithm>
#include <limits>

namespace Files
{
    ConstrainedFileStreamBuf::ConstrainedFileStreamBuf(const std::string& fname, std::size_t start, std::size_t length)
        : mOrigin(start)
    {
        mFile.open(fname.c_str());
        mSize  = length != std::numeric_limits<std::size_t>::max() ? length : mFile.size () - start;

        if (start != 0)
            mFile.seek(start);

        setg(nullptr, nullptr, nullptr);
    }

    std::streambuf::int_type ConstrainedFileStreamBuf::underflow()
    {
        if (gptr() == egptr())
        {
            const std::size_t toRead = std::min((mOrigin + mSize) - (mFile.tell()), sizeof(mBuffer));
            // Read in the next chunk of data, and set the read pointers on success
            // Failure will throw exception in LowLevelFile
            const std::size_t got = mFile.read(mBuffer, toRead);
            setg(&mBuffer[0], &mBuffer[0], &mBuffer[0] + got);
        }
        if (gptr() == egptr())
            return traits_type::eof();

        return traits_type::to_int_type(*gptr());
    }

    std::streambuf::pos_type ConstrainedFileStreamBuf::seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode)
    {
        if ((mode & std::ios_base::out) || !(mode & std::ios_base::in))
            return traits_type::eof();

        // new file position, relative to mOrigin
        size_t newPos;
        switch (whence)
        {
            case std::ios_base::beg:
                newPos = offset;
                break;
            case std::ios_base::cur:
                newPos = (mFile.tell() - mOrigin - (egptr() - gptr())) + offset;
                break;
            case std::ios_base::end:
                newPos = mSize + offset;
                break;
            default:
                return traits_type::eof();
        }

        if (newPos > mSize)
            return traits_type::eof();

        mFile.seek(mOrigin + newPos);

        // Clear read pointers so underflow() gets called on the next read attempt.
        setg(nullptr, nullptr, nullptr);

        return newPos;
    }

    std::streambuf::pos_type ConstrainedFileStreamBuf::seekpos(pos_type pos, std::ios_base::openmode mode)
    {
        if ((mode & std::ios_base::out) || !(mode & std::ios_base::in))
            return traits_type::eof();

        if (static_cast<std::size_t>(pos) > mSize)
            return traits_type::eof();

        mFile.seek(mOrigin + pos);

        // Clear read pointers so underflow() gets called on the next read attempt.
        setg(nullptr, nullptr, nullptr);
        return pos;
    }
}
