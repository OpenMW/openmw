#include "constrainedfilestream.hpp"

namespace Files
{
    IStreamPtr openConstrainedFileStream(const std::string& filename, std::size_t start, std::size_t length)
    {
        return std::make_unique<ConstrainedFileStream>(std::make_unique<ConstrainedFileStreamBuf>(filename, start, length));
    }
}
