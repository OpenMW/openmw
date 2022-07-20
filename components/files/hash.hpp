#ifndef COMPONENTS_FILES_HASH_H
#define COMPONENTS_FILES_HASH_H

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>

namespace Files
{
    std::array<std::uint64_t, 2> getHash(const std::string& fileName, std::istream& stream);
}

#endif
