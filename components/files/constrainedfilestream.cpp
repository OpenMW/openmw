#include <filesystem>

#include "constrainedfilestream.hpp"

namespace Files
{
    IStreamPtr openConstrainedFileStream(const std::filesystem::path& filename, std::size_t start, std::size_t length)
    {
        return std::make_unique<ConstrainedFileStream>(
            std::make_unique<ConstrainedFileStreamBuf>(filename, start, length));
    }
}
