#include "openfile.hpp"

#include <cstring>
#include <fstream>

#if defined(_WIN32) || defined(__WINDOWS__)
#include <boost/locale.hpp>
#endif

namespace Files
{
    std::unique_ptr<std::ifstream> openBinaryInputFileStream(const std::filesystem::path &path)
    {
#if defined(_WIN32) || defined(__WINDOWS__)
        std::wstring wpath = boost::locale::conv::utf_to_utf<wchar_t>(path);
        auto stream = std::make_unique<std::ifstream>(wpath, std::ios::binary);
#else
        auto stream = std::make_unique<std::ifstream>(path, std::ios::binary);
#endif
        if (!stream->is_open())
            throw std::runtime_error("Failed to open '" + path.string() + "' for reading: " + std::strerror(errno)); //TODO(Project579): This will probably break in windows with unicode paths
        stream->exceptions(std::ios::badbit);
        return stream;
    }
}
