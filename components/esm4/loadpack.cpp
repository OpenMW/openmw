/*
  Copyright (C) 2020-2021 cc9cii

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
#include "loadpack.hpp"

#include <stdexcept>
#include <cstring>
//#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::AIPackage::AIPackage() : mFormId(0), mFlags(0)
{
    mEditorId.clear();

    std::memset(&mData, 0, sizeof(PKDT));
    std::memset(&mSchedule, 0, sizeof(PSDT));
    std::memset(&mLocation, 0, sizeof(PLDT));
    mLocation.type = 0xff; // default to indicate no location data
    std::memset(&mTarget, 0, sizeof(PTDT));
    mTarget.type = 0xff;   // default to indicate no target data

    mConditions.clear();
}

ESM4::AIPackage::~AIPackage()
{
}

void ESM4::AIPackage::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_PKDT:
            {
                if (subHdr.dataSize != sizeof(PKDT) && subHdr.dataSize == 4)
                {
                    //std::cout << "skip fallout" << mEditorId << std::endl; // FIXME
                    reader.get(mData.flags);
                    mData.type = 0; // FIXME
                }
                else if (subHdr.dataSize != sizeof(mData))
                    reader.skipSubRecordData(); // FIXME: FO3
                else
                    reader.get(mData);

                break;
            }
            case ESM4::SUB_PSDT: //reader.get(mSchedule); break;
            {
                if (subHdr.dataSize != sizeof(mSchedule))
                    reader.skipSubRecordData(); // FIXME:
                else
                    reader.get(mSchedule); // TES4

                break;
            }
            case ESM4::SUB_PLDT:
            {
                if (subHdr.dataSize != sizeof(mLocation))
                    reader.skipSubRecordData(); // FIXME:
                else
                {
                    reader.get(mLocation); // TES4
                    if (mLocation.type != 5)
                        reader.adjustFormId(mLocation.location);
                }

                break;
            }
            case ESM4::SUB_PTDT:
            {
                if (subHdr.dataSize != sizeof(mTarget))
                    reader.skipSubRecordData(); // FIXME: FO3
                else
                {
                    reader.get(mTarget); // TES4
                    if (mLocation.type != 2)
                        reader.adjustFormId(mTarget.target);
                }

                break;
            }
            case ESM4::SUB_CTDA:
            {
                if (subHdr.dataSize != sizeof(CTDA))
                {
                    reader.skipSubRecordData(); // FIXME: FO3
                    break;
                }

                static CTDA condition;
                reader.get(condition);
                // FIXME: how to "unadjust" if not FormId?
                //adjustFormId(condition.param1);
                //adjustFormId(condition.param2);
                mConditions.push_back(condition);

                break;
            }
            case ESM4::SUB_CTDT: // always 20 for TES4
            case ESM4::SUB_TNAM: // FO3
            case ESM4::SUB_INAM: // FO3
            case ESM4::SUB_CNAM: // FO3
            case ESM4::SUB_SCHR: // FO3
            case ESM4::SUB_POBA: // FO3
            case ESM4::SUB_POCA: // FO3
            case ESM4::SUB_POEA: // FO3
            case ESM4::SUB_SCTX: // FO3
            case ESM4::SUB_SCDA: // FO3
            case ESM4::SUB_SCRO: // FO3
            case ESM4::SUB_IDLA: // FO3
            case ESM4::SUB_IDLC: // FO3
            case ESM4::SUB_IDLF: // FO3
            case ESM4::SUB_IDLT: // FO3
            case ESM4::SUB_PKDD: // FO3
            case ESM4::SUB_PKD2: // FO3
            case ESM4::SUB_PKPT: // FO3
            case ESM4::SUB_PKED: // FO3
            case ESM4::SUB_PKE2: // FO3
            case ESM4::SUB_PKAM: // FO3
            case ESM4::SUB_PUID: // FO3
            case ESM4::SUB_PKW3: // FO3
            case ESM4::SUB_PTD2: // FO3
            case ESM4::SUB_PLD2: // FO3
            case ESM4::SUB_PKFD: // FO3
            case ESM4::SUB_SLSD: // FO3
            case ESM4::SUB_SCVR: // FO3
            case ESM4::SUB_SCRV: // FO3
            case ESM4::SUB_IDLB: // FO3
            case ESM4::SUB_ANAM: // TES5
            case ESM4::SUB_BNAM: // TES5
            case ESM4::SUB_FNAM: // TES5
            case ESM4::SUB_PNAM: // TES5
            case ESM4::SUB_QNAM: // TES5
            case ESM4::SUB_UNAM: // TES5
            case ESM4::SUB_XNAM: // TES5
            case ESM4::SUB_PDTO: // TES5
            case ESM4::SUB_PTDA: // TES5
            case ESM4::SUB_PFOR: // TES5
            case ESM4::SUB_PFO2: // TES5
            case ESM4::SUB_PRCB: // TES5
            case ESM4::SUB_PKCU: // TES5
            case ESM4::SUB_PKC2: // TES5
            case ESM4::SUB_CITC: // TES5
            case ESM4::SUB_CIS1: // TES5
            case ESM4::SUB_CIS2: // TES5
            case ESM4::SUB_VMAD: // TES5
            case ESM4::SUB_TPIC: // TES5
            {
                //std::cout << "PACK " << ESM::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::PACK::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::AIPackage::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::AIPackage::blank()
//{
//}
