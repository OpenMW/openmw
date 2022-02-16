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
#ifndef ESM4_REGN_H
#define ESM4_REGN_H

#include <cstdint>
#include <string>
#include <vector>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Region
    {
        enum RegionDataType
        {
            RDAT_None      = 0x00,
            RDAT_Objects   = 0x02,
            RDAT_Weather   = 0x03,
            RDAT_Map       = 0x04,
            RDAT_Landscape = 0x05,
            RDAT_Grass     = 0x06,
            RDAT_Sound     = 0x07,
            RDAT_Imposter  = 0x08
        };

#pragma pack(push, 1)
        struct RegionData
        {
            std::uint32_t type;
            std::uint8_t  flag;
            std::uint8_t  priority;
            std::uint16_t unknown;
        };

        struct RegionSound
        {
            FormId sound;
            std::uint32_t flags; // 0 pleasant, 1 cloudy, 2 rainy, 3 snowy
            std::uint32_t chance;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string   mEditorId;
        std::uint32_t mColour; // RGBA
        FormId  mWorldId;      // worldspace formid

        std::string   mShader; //?? ICON
        std::string   mMapName;

        std::uint32_t mEdgeFalloff;
        std::vector<std::uint32_t> mRPLD; // unknown, point data?

        RegionData mData;
        std::vector<RegionSound> mSounds;

        Region();
        virtual ~Region();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        void blank();
    };
}

#endif // ESM4_REGN_H
