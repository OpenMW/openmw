#include "openfile.hpp"

#include <cstring>

namespace Files
{
    std::unique_ptr<boost::filesystem::ifstream> openBinaryInputFileStream(const std::string& path)
    {
        auto stream = std::make_unique<boost::filesystem::ifstream>(path, boost::filesystem::fstream::binary);

        if (!stream->is_open())
            throw std::runtime_error("Failed to open '" + path + "' for reading: " + std::strerror(errno));
        stream->exceptions(boost::filesystem::fstream::badbit);
        return stream;
    }
}
