#ifndef OPENMW_ESM_DEFS_H
#define OPENMW_ESM_DEFS_H

#include <libs/platform/stdint.h>

namespace ESM
{

// Pixel color value. Standard four-byte rr,gg,bb,aa format.
typedef int32_t Color;

enum VarType
{
    VT_Unknown,
    VT_None,
    VT_Short,
    VT_Int,
    VT_Long,
    VT_Float,
    VT_String,
    VT_Ignored
};

enum Specialization
{
    SPC_Combat = 0,
    SPC_Magic = 1,
    SPC_Stealth = 2
};

enum RangeType
{
    RT_Self = 0,
    RT_Touch = 1,
    RT_Target = 2
};

#pragma pack(push)
#pragma pack(1)

// Position and rotation
struct Position
{
    float pos[3];
    float rot[3];
};
#pragma pack(pop)

}
#endif
