#ifndef BSA_BA2_FILE_H
#define BSA_BA2_FILE_H

#include <cstdint>
#include <string>

namespace Bsa
{
    uint32_t generateHash(const std::string& name);
    uint32_t generateExtensionHash(std::string_view extension);

    enum class BA2Version : std::uint32_t
    {
        Fallout4 = 1,
        StarfieldGeneral = 2,
        StarfieldDDS = 3,
        Fallout4NextGen_v7 = 7,
        Fallout4NextGen_v8 = 8,
    };
}

#endif
