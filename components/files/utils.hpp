#ifndef COMPONENTS_FILES_UTILS_H
#define COMPONENTS_FILES_UTILS_H

#include <cerrno>
#include <format>
#include <istream>
#include <stdexcept>
#include <system_error>

namespace Files
{
    inline std::streamsize getStreamSizeLeft(std::istream& stream)
    {
        const auto begin = stream.tellg();
        if (stream.fail())
            throw std::runtime_error(
                std::format("Failed to get current file position: {}", std::generic_category().message(errno)));

        stream.seekg(0, std::ios_base::end);
        if (stream.fail())
            throw std::runtime_error(
                std::format("Failed to seek end file position: {}", std::generic_category().message(errno)));

        const auto end = stream.tellg();
        if (stream.fail())
            throw std::runtime_error(
                std::format("Failed to get current file position: {}", std::generic_category().message(errno)));

        stream.seekg(begin);
        if (stream.fail())
            throw std::runtime_error(
                std::format("Failed to seek original file position: {}", std::generic_category().message(errno)));

        return end - begin;
    }
}

#endif
