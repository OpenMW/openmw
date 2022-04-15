#include "openfile.hpp"

#include <cstring>
#include <fstream>

namespace Files
{
    std::unique_ptr<std::ifstream> openBinaryInputFileStream(const std::string& path)
    {
        auto stream = std::make_unique<std::ifstream>(path, std::ios::binary);
        if (!stream->is_open())
            throw std::runtime_error("Failed to open '" + path + "' for reading: " + std::strerror(errno));
        stream->exceptions(std::ios::badbit);
        return stream;
    }
}
