/*
  Copyright (C) 2015-2016, 2018, 2020 cc9cii

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
#ifndef ESM4_LAND_H
#define ESM4_LAND_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;

    struct Land
    {
        enum
        {
            LAND_VNML = 1,
            LAND_VHGT = 2,
            LAND_WNAM = 4, // only in TES3?
            LAND_VCLR = 8,
            LAND_VTEX = 16
        };

        // number of vertices per side
        static const int VERTS_PER_SIDE = 33;

        // cell terrain size in world coords
        static const int REAL_SIZE = 4096;

        // total number of vertices
        static const int LAND_NUM_VERTS = VERTS_PER_SIDE * VERTS_PER_SIDE;

        static const int HEIGHT_SCALE = 8;

        // number of textures per side of a land quadrant
        // (for TES4 - based on vanilla observations)
        static const int QUAD_TEXTURE_PER_SIDE = 6;

#pragma pack(push,1)
        struct VHGT
        {
            float         heightOffset;
            std::int8_t   gradientData[VERTS_PER_SIDE * VERTS_PER_SIDE];
            std::uint16_t unknown1;
            unsigned char unknown2;
        };

        struct BTXT
        {
            FormId        formId;
            std::uint8_t  quadrant; // 0 = bottom left, 1 = bottom right, 2 = top left, 3 = top right
            std::uint8_t  unknown1;
            std::uint16_t unknown2;
        };

        struct ATXT
        {
            FormId        formId;
            std::uint8_t  quadrant; // 0 = bottom left, 1 = bottom right, 2 = top left, 3 = top right
            std::uint8_t  unknown;
            std::uint16_t layerIndex; // texture layer, 0..7
        };

        struct VTXT
        {
            std::uint16_t position; // 0..288 (17x17 grid)
            std::uint8_t  unknown1;
            std::uint8_t  unknown2;
            float         opacity;
        };
#pragma pack(pop)

        struct TxtLayer
        {
            ATXT          texture;
            std::vector<VTXT> data; // alpha data
        };

        struct Texture
        {
            BTXT          base;
            std::vector<TxtLayer> layers;
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::uint32_t mLandFlags; // from DATA subrecord

        // FIXME: lazy loading not yet implemented
        int mDataTypes; // which data types are loaded

        signed char   mVertNorm[VERTS_PER_SIDE * VERTS_PER_SIDE * 3]; // from VNML subrecord
        signed char   mVertColr[VERTS_PER_SIDE * VERTS_PER_SIDE * 3]; // from VCLR subrecord
        VHGT          mHeightMap;
        Texture       mTextures[4]; // 0 = bottom left, 1 = bottom right, 2 = top left, 3 = top right
        std::vector<FormId> mIds;   // land texture (LTEX) formids

        Land();
        virtual ~Land();

        virtual void load(Reader& reader);
        //virtual void save(Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_LAND_H
