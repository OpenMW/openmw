/*
  Copyright (C) 2015-2016, 2018-2021 cc9cii

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
#include "loadwrld.hpp"

#include <stdexcept>

#include "reader.hpp"
// #include "writer.hpp"

void ESM4::World::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    // It should be possible to save the current world formId automatically while reading in
    // the record header rather than doing it manually here but possibly less efficient (may
    // need to check each record?).
    //
    // Alternatively it may be possible to figure it out by examining the group headers, but
    // apparently the label field is not reliable so the parent world formid may have been
    // corrupted by the use of ignore flag (TODO: should check to verify).
    reader.setCurrWorld(mId); // save for CELL later

    std::uint32_t esmVer = reader.esmVersion();
    // bool isTES4 = (esmVer == ESM::VER_080 || esmVer == ESM::VER_100);
    // bool isFONV = (esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134);
    bool isTES5 = (esmVer == ESM::VER_094 || esmVer == ESM::VER_170); // WARN: FO3 is also VER_094
    bool usingDefaultLevels = true;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("WCTR"): // TES5+
                reader.get(mCenterCell);
                break;
            case ESM::fourCC("WNAM"):
                reader.getFormId(mParent);
                break;
            case ESM::fourCC("SNAM"):
                reader.get(mSound);
                break; // sound, Oblivion only?
            case ESM::fourCC("ICON"):
                reader.getZString(mMapFile);
                break;
            case ESM::fourCC("CNAM"):
                reader.getFormId(mClimate);
                break;
            case ESM::fourCC("NAM2"):
                reader.getFormId(mWater);
                break;
            case ESM::fourCC("NAM0"):
            {
                reader.get(mMinX);
                reader.get(mMinY);
                break;
            }
            case ESM::fourCC("NAM9"):
            {
                reader.get(mMaxX);
                reader.get(mMaxY);
                break;
            }
            case ESM::fourCC("DATA"):
                reader.get(mWorldFlags);
                break;
            case ESM::fourCC("MNAM"):
            {
                reader.get(mMap.width);
                reader.get(mMap.height);
                reader.get(mMap.NWcellX);
                reader.get(mMap.NWcellY);
                reader.get(mMap.SEcellX);
                reader.get(mMap.SEcellY);

                if (subHdr.dataSize == 28) // Skyrim?
                {
                    reader.get(mMap.minHeight);
                    reader.get(mMap.maxHeight);
                    reader.get(mMap.initialPitch);
                }

                break;
            }
            case ESM::fourCC("DNAM"): // defaults
            {
                reader.get(mLandLevel); //  -2700.f for TES5
                reader.get(mWaterLevel); // -14000.f for TES5
                usingDefaultLevels = false;

                break;
            }
            // Only a few worlds in FO3 have music (I'm guessing 00090908 "explore" is the default?)
            // 00090906 public  WRLD: 00000A74 MegatonWorld
            // 00090CE7 base    WRLD: 0001A25D DCWorld18 (Arlington National Cemeteray)
            // 00090CE7 base    WRLD: 0001A266 DCWorld09 (The Mall)
            // 00090CE7 base    WRLD: 0001A267 DCWorld08 (Pennsylvania Avenue)
            // 000BAD30 tranquilitylane WRLD: 000244A7 TranquilityLane
            // 00090CE7 base    WRLD: 000271C0 MonumentWorld (The Washington Monument)
            // 00090907 dungeon WRLD: 0004C4D1 MamaDolcesWorld (Mama Dolce's Loading Yard)
            //
            // FONV has only 3 (note the different format, also can't find the files?):
            // 00119D2E freeside\freeside_01.mp3 0010BEEA FreesideWorld (Freeside)
            // 00119D2E freeside\freeside_01.mp3 0012D94D FreesideNorthWorld (Freeside)
            // 00119D2E freeside\freeside_01.mp3 0012D94E FreesideFortWorld (Old Mormon Fort)
            // NOTE: FONV DefaultObjectManager has 00090908 "explore" as the default music
            case ESM::fourCC("ZNAM"):
                reader.getFormId(mMusic);
                break;
            case ESM::fourCC("PNAM"):
                reader.get(mParentUseFlags);
                break;
            case ESM::fourCC("OFST"):
            case ESM::fourCC("RNAM"): // multiple
            case ESM::fourCC("MHDT"):
            case ESM::fourCC("LTMP"):
            case ESM::fourCC("XEZN"):
            case ESM::fourCC("XLCN"):
            case ESM::fourCC("NAM3"):
            case ESM::fourCC("NAM4"):
            case ESM::fourCC("NAMA"):
            case ESM::fourCC("ONAM"):
            case ESM::fourCC("TNAM"):
            case ESM::fourCC("UNAM"):
            case ESM::fourCC("XWEM"):
            case ESM::fourCC("MODL"): // Model data start
            case ESM::fourCC("MODT"):
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("INAM"): // FO3
            case ESM::fourCC("NNAM"): // FO3
            case ESM::fourCC("XNAM"): // FO3
            case ESM::fourCC("IMPS"): // FO3 Anchorage
            case ESM::fourCC("IMPF"): // FO3 Anchorage
            case ESM::fourCC("CLSZ"): // FO4
            case ESM::fourCC("WLEV"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::WRLD::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }

        if (usingDefaultLevels)
        {
            if (isTES5)
            {
                mLandLevel = -2700.f;
                mWaterLevel = -14000.f;
            }
            else
            {
                mLandLevel = 0.f; // FIXME: not sure that this value is correct
                mWaterLevel = 0.f;
            }
        }

        // TES4 doesn't define PNAM. Exact parent worldspace behavior needs research
        if (!reader.hasFormVersion())
            mParentUseFlags = 0xFFFF;
    }
}

// void ESM4::World::save(ESM4::Writer& writer) const
//{
// }
