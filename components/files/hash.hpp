#ifndef COMPONENTS_FILES_HASH_H
#define COMPONENTS_FILES_HASH_H

#include <cstdint>
#include <istream>
#include <string>

namespace Files
{
    std::uint64_t getHash(const std::string& fileName, std::istream& stream);
}

#endif
