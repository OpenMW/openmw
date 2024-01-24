#ifndef BSA_BA2_FILE_H
#define BSA_BA2_FILE_H

#include <cstdint>
#include <string>

namespace Bsa
{
    uint32_t generateHash(const std::string& name);
    uint32_t generateExtensionHash(std::string_view extension);
}

#endif
