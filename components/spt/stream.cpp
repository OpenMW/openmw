#include "stream.hpp"

#include <cstdint>
#include <format>
#include <stdexcept>

namespace Spt
{
    int32_t SPTStream::readSection()
    {
        int32_t value;
        read(value);
        return value;
    }

    void SPTStream::expectSection(int32_t expected)
    {
        const int32_t section = readSection();
        if (section != expected)
            throw std::runtime_error(std::format("Expected section {}, got {}", expected, section));
    }
}
