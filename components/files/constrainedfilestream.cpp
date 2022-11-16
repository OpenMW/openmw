#include "constrainedfilestream.hpp"

#include <filesystem>
#include <memory>

#include <components/files/constrainedfilestreambuf.hpp>
#include <components/files/istreamptr.hpp>
#include <components/files/streamwithbuffer.hpp>

namespace Files
{
    IStreamPtr openConstrainedFileStream(const std::filesystem::path& filename, std::size_t start, std::size_t length)
    {
        return std::make_unique<ConstrainedFileStream>(
            std::make_unique<ConstrainedFileStreamBuf>(filename, start, length));
    }
}
