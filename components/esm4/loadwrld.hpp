/*
  Copyright (C) 2015-2016, 2018-2020 cc9cii

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
#ifndef ESM4_WRLD_H
#define ESM4_WRLD_H

#include <cstdint>
#include <string>
#include <vector>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>

#include "grid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct World
    {
        enum WorldFlags // TES4                 TES5
        { // -------------------- -----------------
            WLD_Small = 0x01, // Small World          Small World
            WLD_NoFastTravel = 0x02, // Can't Fast Travel    Can't Fast Travel
            WLD_Oblivion = 0x04, // Oblivion worldspace
            WLD_NoLODWater = 0x08, //                      No LOD Water
            WLD_NoLandscpe = 0x10, // No LOD Water         No Landscape
            WLD_NoSky = 0x20, //                      No Sky
            wLD_FixedDimension = 0x40, //                      Fixed Dimensions
            WLD_NoGrass = 0x80 //                      No Grass
        };

        enum UseFlags
        {
            UseFlag_Land = 0x01,
            UseFlag_LOD = 0x02,
            UseFlag_Map = 0x04,
            UseFlag_Water = 0x08,
            UseFlag_Climate = 0x10,
            UseFlag_Imagespace = 0x20, // Unused in TES5
            UseFlag_SkyCell = 0x40,
            // cc9cii: 0x80 == needs water adjustment? Set for WastelandNV
        };

        struct REFRcoord
        {
            ESM::FormId formId;
            std::int16_t unknown1;
            std::int16_t unknown2;
        };

        struct RNAMstruct
        {
            std::int16_t unknown1;
            std::int16_t unknown2;
            std::vector<REFRcoord> refrs;
        };

        // Map size struct, either 16 (old format) or 28 (new format) byte structure
        struct Map
        {
            std::uint32_t width; // usable width of the map
            std::uint32_t height; // usable height of the map
            std::int16_t NWcellX;
            std::int16_t NWcellY;
            std::int16_t SEcellX;
            std::int16_t SEcellY;
            float minHeight; // Camera Data (default 50000), new as of Skyrim 1.8, purpose is not yet known.
            float maxHeight; // Camera Data (default 80000)
            float initialPitch;
        };

        ESM::FormId mId; // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        ESM::FormId mParent; // parent worldspace formid
        std::uint8_t mWorldFlags;
        ESM::FormId mClimate;
        ESM::FormId mWater;
        float mLandLevel;
        float mWaterLevel;

        Map mMap;

        std::int32_t mMinX;
        std::int32_t mMinY;
        std::int32_t mMaxX;
        std::int32_t mMaxY;

        // ------ TES4 only -----

        std::int32_t mSound; // 0 = no record, 1 = Public, 2 = Dungeon
        std::string mMapFile;

        // ------ TES5 only -----

        Grid mCenterCell;
        RNAMstruct mData;

        // ----------------------
        ESM::FormId mMusic;

        std::uint16_t mParentUseFlags{ 0 };

        // cache formId's of children (e.g. CELL, ROAD)
        std::vector<ESM::FormId> mCells;
        std::vector<ESM::FormId> mRoads;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;
        static constexpr ESM::RecNameInts sRecordId = ESM::REC_WRLD4;
    };
}

#endif // ESM4_WRLD_H
