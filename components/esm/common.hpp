#ifndef COMPONENT_ESM_COMMON_H
#define COMPONENT_ESM_COMMON_H

#include <cstdint>
#include <string>

namespace ESM
{
#pragma pack(push, 1)
    union ESMVersion
    {
        float f;
        std::uint32_t ui;
    };

    union TypeId
    {
        std::uint32_t value;
        char name[4]; // record type in ascii
    };
#pragma pack(pop)

    enum ESMVersions
    {
        VER_120 = 0x3f99999a, // TES3
        VER_130 = 0x3fa66666, // TES3
        VER_080 = 0x3f4ccccd, // TES4
        VER_100 = 0x3f800000, // TES4, FO4
        VER_132 = 0x3fa8f5c3, // FONV Courier's Stash, DeadMoney
        VER_133 = 0x3faa3d71, // FONV HonestHearts
        VER_134 = 0x3fab851f, // FONV, GunRunnersArsenal, LonesomeRoad, OldWorldBlues
        VER_094 = 0x3f70a3d7, // TES5/FO3
        VER_170 = 0x3fd9999a, // TES5
        VER_171 = 0x3fdae148, // TES5
        VER_095 = 0x3f733333, // FO4
    };

    // Defines another files (esm or esp) that this file depends upon.
    struct MasterData
    {
        std::string name;
        std::uint64_t size;
    };

    inline std::string printName(const std::uint32_t typeId)
    {
        unsigned char typeName[4];
        typeName[0] = typeId & 0xff;
        typeName[1] = (typeId >> 8) & 0xff;
        typeName[2] = (typeId >> 16) & 0xff;
        typeName[3] = (typeId >> 24) & 0xff;

        return std::string(reinterpret_cast<char*>(typeName), 4);
    }
}

#endif // COMPONENT_ESM_COMMON_H
