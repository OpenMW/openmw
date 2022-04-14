#include "constrainedfilestream.hpp"

namespace Files
{
    ConstrainedFileStream::ConstrainedFileStream(std::unique_ptr<ConstrainedFileStreamBuf> buf)
        : std::istream(buf.get())
        , mBuf(std::move(buf))
    {
    }

    IStreamPtr openConstrainedFileStream(const std::string& filename, std::size_t start, std::size_t length)
    {
        return std::make_shared<ConstrainedFileStream>(std::make_unique<ConstrainedFileStreamBuf>(filename, start, length));
    }
}
