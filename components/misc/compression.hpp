#ifndef OPENMW_COMPONENTS_MISC_COMPRESSION_H
#define OPENMW_COMPONENTS_MISC_COMPRESSION_H

#include <cstddef>
#include <vector>

namespace Misc
{
    std::vector<std::byte> compress(const std::vector<std::byte>& data);

    std::vector<std::byte> decompress(const std::vector<std::byte>& data);
}

#endif
