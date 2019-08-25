/*
  Copyright (C) 2015-2016, 2018-2019 cc9cii

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
#include "wrld.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::World::World() : mFormId(0), mFlags(0), mParent(0), mWorldFlags(0), mClimate(0), mWater(0),
                       mLandLevel(-2700.f), mWaterLevel(-14000.f),
                       mMinX(0), mMinY(0), mMaxX(0), mMaxY(0), mSound(0)
{
    mEditorId.clear();
    mFullName.clear();
    mMapFile.clear();

    mMap.width = 0;
    mMap.height = 0;
    mMap.NWcellX = 0;
    mMap.NWcellY = 0;
    mMap.SEcellX = 0;
    mMap.SEcellY = 0;
    mMap.minHeight = 0.f;
    mMap.maxHeight = 0.f;
    mMap.initialPitch = 0.f;
}

ESM4::World::~World()
{
}

void ESM4::World::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    // It should be possible to save the current world formId automatically while reading in
    // the record header rather than doing it manually here but possibly less efficient (may
    // need to check each record?).
    //
    // Alternatively it may be possible to figure it out by examining the group headers, but
    // apparently the label field is not reliable so the parent world formid may have been
    // corrupted by the use of ignore flag (TODO: should check to verify).
    reader.setCurrWorld(mFormId); // save for CELL later

    std::uint32_t subSize = 0; // for XXXX sub record

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_FULL: // Name of the worldspace
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("WRLD FULL data read error");

                break;
            }
            case ESM4::SUB_WCTR: reader.get(mCenterCell);   break; // Center cell, TES5 only
            case ESM4::SUB_WNAM: reader.getFormId(mParent); break;
            case ESM4::SUB_SNAM: reader.get(mSound);        break; // sound, Oblivion only?
            case ESM4::SUB_ICON: reader.getZString(mMapFile); break; // map filename, Oblivion only?
            case ESM4::SUB_CNAM: reader.get(mClimate);      break;
            case ESM4::SUB_NAM2: reader.getFormId(mWater);  break;
            case ESM4::SUB_NAM0:
            {
                reader.get(mMinX);
                reader.get(mMinY);
                break;
            }
            case ESM4::SUB_NAM9:
            {
                reader.get(mMaxX);
                reader.get(mMaxY);
                break;
            }
            case ESM4::SUB_DATA: reader.get(mWorldFlags);   break;
            case ESM4::SUB_MNAM:
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
            case ESM4::SUB_DNAM:
            {
                reader.get(mLandLevel);
                reader.get(mWaterLevel);
                break;
            }
            case ESM4::SUB_RNAM: // multiple
            case ESM4::SUB_MHDT:
            case ESM4::SUB_LTMP:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_NAM3:
            case ESM4::SUB_NAM4:
            case ESM4::SUB_MODL:
            case ESM4::SUB_NAMA:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_ONAM:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_UNAM:
            case ESM4::SUB_ZNAM:
            case ESM4::SUB_XWEM:
            case ESM4::SUB_MODT: // from Dragonborn onwards?
            case ESM4::SUB_INAM: // FO3
            case ESM4::SUB_NNAM: // FO3
            case ESM4::SUB_XNAM: // FO3
            case ESM4::SUB_IMPS: // FO3 Anchorage
            case ESM4::SUB_IMPF: // FO3 Anchorage
            {
                //std::cout << "WRLD " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                break;
            }
            case ESM4::SUB_OFST:
            {
                if (subSize)
                {
                    reader.skipSubRecordData(subSize); // special post XXXX
                    reader.updateRecordRemaining(subSize); // WARNING: manually update
                    subSize = 0;
                }
                else
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip

                break;
            }
            case ESM4::SUB_XXXX:
            {
                reader.get(subSize);
                break;
            }
            default:
                throw std::runtime_error("ESM4::WRLD::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::World::save(ESM4::Writer& writer) const
//{
//}
