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
#ifndef ESM4_CELL_H
#define ESM4_CELL_H

#include <cstdint>
#include <string>
#include <vector>

#include <components/esm/defs.hpp>
#include <components/esm/exteriorcelllocation.hpp>
#include <components/esm/refid.hpp>
#include <components/esm4/reader.hpp>
#include <components/misc/constants.hpp>

#include "lighting.hpp"

namespace ESM4
{
    class Reader;
    class Writer;
    struct ReaderContext;
    struct CellGroup;

    enum CellFlags // TES4                     TES5
    { // -----------------------  ------------------------------------
        CELL_Interior = 0x0001, // Can't travel from here   Interior
        CELL_HasWater = 0x0002, // Has water (Int)          Has Water (Int)
        CELL_NoTravel = 0x0004, //                          not Can't Travel From Here(Int only)
        CELL_HideLand = 0x0008, // Force hide land (Ext)    No LOD Water
                                // Oblivion interior (Int)
        CELL_Public = 0x0020, // Public place             Public Area
        CELL_HandChgd = 0x0040, // Hand changed             Hand Changed
        CELL_QuasiExt = 0x0080, // Behave like exterior     Show Sky
        CELL_SkyLight = 0x0100 //                          Use Sky Lighting
    };

    // Unlike TES3, multiple cells can have the same exterior co-ordinates.
    // The cells need to be organised under world spaces.
    struct Cell
    {
        static constexpr int sSize = Constants::ESM4CellSizeInUnits;

        ESM::RefId mId; // from the header
        std::uint32_t mFlags = 0; // from the header, see enum type RecordFlag for details

        ESM::RefId mParent; // world formId (for grouping cells), from the loading sequence

        std::string mEditorId;
        std::string mFullName;
        std::uint16_t mCellFlags = 0; // TES5 can also be 8 bits

        std::int32_t mX = 0;
        std::int32_t mY = 0;

        ESM::FormId mOwner;
        ESM::FormId mGlobal;
        ESM::FormId mClimate;
        ESM::FormId mWater;
        float mWaterHeight = sInvalidWaterLevel;

        std::vector<ESM::FormId> mRegions;
        Lighting mLighting;

        ESM::FormId mLightingTemplate; // FO3/FONV
        std::uint32_t mLightingTemplateFlags = 0; // FO3/FONV

        ESM::FormId mMusic; // FO3/FONV
        ESM::FormId mAcousticSpace; // FO3/FONV
        // TES4: 0 = default, 1 = public, 2 = dungeon
        // FO3/FONV have more types (not sure how they are used, however)
        std::uint8_t mMusicType = 0;

        CellGroup* mCellGroup = nullptr;

        ESM4::ReaderContext mReaderContext;

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        void blank();

        static constexpr ESM::RecNameInts sRecordId = ESM::REC_CELL4;

        int getGridX() const { return mX; }
        int getGridY() const { return mY; }
        bool isExterior() const { return !(mCellFlags & CELL_Interior); }
        ESM::ExteriorCellLocation getExteriorCellLocation() const
        {
            return ESM::ExteriorCellLocation(mX, mY, isExterior() ? mParent : mId);
        }
        static float sInvalidWaterLevel;
    };
}

#endif // ESM4_CELL_H
