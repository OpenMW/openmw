#include "common.hpp"

#include <sstream>

namespace ESM
{
    std::string printName(const std::uint32_t typeId)
    {
        unsigned char typeName[4];
        typeName[0] =  typeId        & 0xff;
        typeName[1] = (typeId >>  8) & 0xff;
        typeName[2] = (typeId >> 16) & 0xff;
        typeName[3] = (typeId >> 24) & 0xff;

        return std::string((char*)typeName, 4);
    }
}
