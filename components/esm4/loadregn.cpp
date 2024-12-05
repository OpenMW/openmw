/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

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
#include "loadregn.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Region::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("RCLR"):
                reader.get(mColour);
                break;
            case ESM::fourCC("WNAM"):
                reader.getFormId(mWorldId);
                break;
            case ESM::fourCC("ICON"):
                reader.getZString(mShader);
                break;
            case ESM::fourCC("RPLI"):
                reader.get(mEdgeFalloff);
                break;
            case ESM::fourCC("RPLD"):
            {
                mRPLD.resize(subHdr.dataSize / sizeof(std::uint32_t));
                for (std::vector<std::uint32_t>::iterator it = mRPLD.begin(); it != mRPLD.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding;
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "RPLD: 0x" << std::hex << *it << std::endl;
#endif
                }

                break;
            }
            case ESM::fourCC("RDAT"):
                reader.get(mData);
                break;
            case ESM::fourCC("RDMP"):
            {
                if (mData.type != RDAT_Map)
                    throw std::runtime_error("REGN unexpected data type");
                reader.getLocalizedString(mMapName);
                break;
            }
            // FO3 only 2: DemoMegatonSound and DC01 (both 0 RDMD)
            // FONV none
            case ESM::fourCC("RDMD"): // music type; 0 default, 1 public, 2 dungeon
            {
#if 0
                int dummy;
                reader.get(dummy);
                std::cout << "REGN " << mEditorId << " " << dummy << std::endl;
#else
                reader.skipSubRecordData();
#endif
                break;
            }
            case ESM::fourCC("RDMO"): // not seen in FO3/FONV?
            {
                // std::cout << "REGN " << ESM::printName(subHdr.typeId) << " skipping..."
                // << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            case ESM::fourCC("RDSD"): // Possibly the same as RDSA
            {
                if (mData.type != RDAT_Sound)
                    throw std::runtime_error(
                        "ESM4::REGN::load - unexpected data type " + ESM::printName(subHdr.typeId));

                std::size_t numSounds = subHdr.dataSize / sizeof(RegionSound);
                mSounds.resize(numSounds);
                for (std::size_t i = 0; i < numSounds; ++i)
                    reader.get(mSounds.at(i));

                break;
            }
            case ESM::fourCC("RDGS"): // Only in Oblivion? (ToddTestRegion1) // formId
            case ESM::fourCC("RDSA"):
            case ESM::fourCC("RDWT"): // formId
            case ESM::fourCC("RDOT"): // formId
            case ESM::fourCC("RDID"): // FONV
            case ESM::fourCC("RDSB"): // FONV
            case ESM::fourCC("RDSI"): // FONV
            case ESM::fourCC("NVMI"): // TES5
            case ESM::fourCC("ANAM"): // FO4
            case ESM::fourCC("RLDM"): // FO4
                // RDAT skipping... following is a map
                // RDMP skipping... map name
                //
                // RDAT skipping... following is weather
                // RDWT skipping... weather data
                //
                // RDAT skipping... following is sound
                // RDMD skipping... unknown, maybe music data
                //
                // RDSD skipping... unknown, maybe sound data
                //
                // RDAT skipping... following is grass
                // RDGS skipping... unknown, maybe grass

                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::REGN::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Region::save(ESM4::Writer& writer) const
//{
// }

void ESM4::Region::blank() {}
