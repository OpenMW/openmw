/*
  Copyright (C) 2015-2016, 2018 cc9cii

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
#include "regn.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

ESM4::Region::Region() : mFormId(0), mFlags(0)
{
    mEditorId.clear();
    mShader.clear();
    mMapName.clear();
    //mData.unknown = 1; // FIXME: temp use to indicate not loaded
    mData.resize(8);
}

ESM4::Region::~Region()
{
}

void ESM4::Region::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    RDAT_Types next = RDAT_None;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_RCLR: reader.get(mColour);        break;
            case ESM4::SUB_WNAM: reader.getFormId(mWorldId); break;
            case ESM4::SUB_ICON: reader.getZString(mShader); break;
            case ESM4::SUB_RPLI: reader.get(mEdgeFalloff);   break;
            case ESM4::SUB_RPLD:
            {
                mRPLD.resize(subHdr.dataSize/sizeof(std::uint32_t));
                for (std::vector<std::uint32_t>::iterator it = mRPLD.begin(); it != mRPLD.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "RPLD: 0x" << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_RDAT:
            {
                RDAT rdat;
                reader.get(rdat);

                next = static_cast<RDAT_Types>(rdat.type);

                mData[rdat.type].type = rdat.type;
                mData[rdat.type].flag = rdat.flag;
                mData[rdat.type].priority = rdat.priority;
                mData[rdat.type].unknown = rdat.unknown;

                break;
            }
            case ESM4::SUB_RDMP:
            {
                assert(next == RDAT_Map && "REGN unexpected data type");
                next = RDAT_None;

                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mMapName);
                else if (!reader.getZString(mMapName))
                    throw std::runtime_error ("REGN RDMP data read error");
                break;
            }
            case ESM4::SUB_RDMD: // Only in Oblivion?
            case ESM4::SUB_RDSD: // Only in Oblivion?  Possibly the same as RDSA // formId
            case ESM4::SUB_RDGS: // Only in Oblivion? (ToddTestRegion1) // formId
            case ESM4::SUB_RDMO:
            case ESM4::SUB_RDSA:
            case ESM4::SUB_RDWT: // formId
            case ESM4::SUB_RDOT: // formId
            case ESM4::SUB_RDID: // FONV
            case ESM4::SUB_RDSB: // FONV
            case ESM4::SUB_RDSI: // FONV
            {
                //RDAT skipping... following is a map
                //RDMP skipping... map name
                //
                //RDAT skipping... following is weather
                //RDWT skipping... weather data
                //
                //RDAT skipping... following is sound
                //RDMD skipping... unknown, maybe music data
                //
                //RDSD skipping... unknown, maybe sound data
                //
                //RDAT skipping... following is grass
                //RDGS skipping... unknown, maybe grass

                //std::cout << "REGN " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                break;
            }
            default:
                throw std::runtime_error("ESM4::REGN::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Region::save(ESM4::Writer& writer) const
//{
//}

void ESM4::Region::blank()
{
}
