#ifndef COMPONENTS_FILES_HASH_H
#define COMPONENTS_FILES_HASH_H

#include <array>
#include <cstdint>
#include <iosfwd>
#include <filesystem>

namespace Files
{
    std::array<std::uint64_t, 2> getHash(const std::filesystem::path& fileName, std::istream& stream);
}

#endif
