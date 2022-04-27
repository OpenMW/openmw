#ifndef COMPONENT_ESM_FORMAT_H
#define COMPONENT_ESM_FORMAT_H

#include "defs.hpp"

#include <cstdint>
#include <istream>
#include <string_view>

namespace ESM
{
    enum class Format : std::uint32_t
    {
        Tes3 = fourCC("TES3"),
        Tes4 = fourCC("TES4"),
    };

    Format readFormat(std::istream& stream);

    Format parseFormat(std::string_view value);
}

#endif
