/*
  Copyright (C) 2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_LIGHTING_H
#define ESM4_LIGHTING_H

#include <cstdint>

namespace ESM4
{
#pragma pack(push, 1)
    // guesses only for TES4
    struct Lighting
    {                              //               | Aichan Prison values
        std::uint32_t ambient;     //               | 16 17 19 00 (RGBA)
        std::uint32_t directional; //               | 00 00 00 00 (RGBA)
        std::uint32_t fogColor;    //               | 1D 1B 16 00 (RGBA)
        float         fogNear;     // Fog Near      | 00 00 00 00 = 0.f
        float         fogFar;      // Fog Far       | 00 80 3B 45 = 3000.f
        std::int32_t  rotationXY;  // rotation xy   | 00 00 00 00 = 0
        std::int32_t  rotationZ;   // rotation z    | 00 00 00 00 = 0
        float         fogDirFade;  // Fog dir fade  | 00 00 80 3F = 1.f
        float         fogClipDist; // Fog clip dist | 00 80 3B 45 = 3000.f
        float         fogPower;
    };

    struct Lighting_TES5
    {
        std::uint32_t ambient;
        std::uint32_t directional;
        std::uint32_t fogColor;
        float         fogNear;
        float         fogFar;
        std::int32_t  rotationXY;
        std::int32_t  rotationZ;
        float         fogDirFade;
        float         fogClipDist;
        float         fogPower;
        std::uint32_t unknown1;
        std::uint32_t unknown2;
        std::uint32_t unknown3;
        std::uint32_t unknown4;
        std::uint32_t unknown5;
        std::uint32_t unknown6;
        std::uint32_t unknown7;
        std::uint32_t unknown8;
        std::uint32_t fogColorFar;
        float         fogMax;
        float         LightFadeStart;
        float         LightFadeEnd;
        std::uint32_t padding;
    };
#pragma pack(pop)
}

#endif // ESM4_LIGHTING_H
