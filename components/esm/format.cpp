#include "format.hpp"

#include <cstring>
#include <istream>
#include <stdexcept>
#include <string>

namespace ESM
{
    namespace
    {
        bool isValidFormat(std::uint32_t value)
        {
            return value == static_cast<std::uint32_t>(Format::Tes3)
                || value == static_cast<std::uint32_t>(Format::Tes4);
        }

        Format toFormat(std::uint32_t value)
        {
            if (!isValidFormat(value))
                throw std::runtime_error("Invalid format: " + std::to_string(value));
            return static_cast<Format>(value);
        }
    }

    Format readFormat(std::istream& stream)
    {
        std::uint32_t format = 0;
        stream.read(reinterpret_cast<char*>(&format), sizeof(format));
        if (stream.gcount() != sizeof(format))
            throw std::runtime_error("Not enough bytes to read file header");
        return toFormat(format);
    }

    Format parseFormat(std::string_view value)
    {
        if (value.size() != sizeof(std::uint32_t))
            throw std::logic_error("Invalid format value: " + std::string(value));
        std::uint32_t format;
        std::memcpy(&format, value.data(), sizeof(std::uint32_t));
        return toFormat(format);
    }
}
