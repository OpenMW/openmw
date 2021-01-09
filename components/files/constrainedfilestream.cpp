#include "constrainedfilestream.hpp"

#include <streambuf>
#include <algorithm>

#include "lowlevelfile.hpp"

namespace
{
// somewhat arbitrary though 64KB buffers didn't seem to improve performance any
const size_t sBufferSize = 8192;
}

namespace Files
{
    class ConstrainedFileStreamBuf : public std::streambuf
    {

        size_t mOrigin;
        size_t mSize;

        LowLevelFile mFile;

        char mBuffer[sBufferSize]{0};

    public:
        ConstrainedFileStreamBuf(const std::string &fname, size_t start, size_t length)
        {
            mFile.open (fname.c_str ());
            mSize  = length != 0xFFFFFFFF ? length : mFile.size () - start;

            if (start != 0)
                mFile.seek(start);

            setg(nullptr,nullptr,nullptr);

            mOrigin = start;
        }

        int_type underflow() override
        {
            if(gptr() == egptr())
            {
                size_t toRead = std::min((mOrigin+mSize)-(mFile.tell()), sBufferSize);
                // Read in the next chunk of data, and set the read pointers on success
                // Failure will throw exception in LowLevelFile
                size_t got = mFile.read(mBuffer, toRead);
                setg(&mBuffer[0], &mBuffer[0], &mBuffer[0]+got);
            }
            if(gptr() == egptr())
                return traits_type::eof();

            return traits_type::to_int_type(*gptr());
        }

        pos_type seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode) override
        {
            if((mode&std::ios_base::out) || !(mode&std::ios_base::in))
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

            mFile.seek(mOrigin+newPos);

            // Clear read pointers so underflow() gets called on the next read attempt.
            setg(nullptr, nullptr, nullptr);

            return newPos;
        }

        pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
        {
            if((mode&std::ios_base::out) || !(mode&std::ios_base::in))
                return traits_type::eof();

            if ((size_t)pos > mSize)
                return traits_type::eof();

            mFile.seek(mOrigin + pos);

            // Clear read pointers so underflow() gets called on the next read attempt.
            setg(nullptr, nullptr, nullptr);
            return pos;
        }

    };

    ConstrainedFileStream::ConstrainedFileStream(std::unique_ptr<std::streambuf> buf)
        : std::istream(buf.get())
        , mBuf(std::move(buf))
    {
    }

    IStreamPtr openConstrainedFileStream(const char *filename,
                                                       size_t start, size_t length)
    {
        auto buf = std::unique_ptr<std::streambuf>(new ConstrainedFileStreamBuf(filename, start, length));
        return IStreamPtr(new ConstrainedFileStream(std::move(buf)));
    }
}
