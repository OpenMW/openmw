#include "openfile.hpp"
#include "conversion.hpp"

#include <cstring>
#include <fstream>

namespace Files
{
    std::unique_ptr<std::ifstream> openBinaryInputFileStream(const std::filesystem::path& path)
    {
        auto stream = std::make_unique<std::ifstream>(path, std::ios::binary);
        if (!stream->is_open())
            throw std::runtime_error(
                "Failed to open '" + Files::pathToUnicodeString(path) + "' for reading: " + std::strerror(errno));
        stream->exceptions(std::ios::badbit);
        return stream;
    }
}
