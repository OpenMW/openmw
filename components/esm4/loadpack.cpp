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

#include <cstring>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::AIPackage::load(ESM4::Reader& reader)
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
            case ESM::fourCC("PKDT"):
            {
                if (subHdr.dataSize != sizeof(PKDT) && subHdr.dataSize == 4)
                {
                    // std::cout << "skip fallout" << mEditorId << std::endl; // FIXME
                    reader.get(mData.flags);
                    mData.type = 0; // FIXME
                }
                else if (subHdr.dataSize != sizeof(mData))
                    reader.skipSubRecordData(); // FIXME: FO3
                else
                    reader.get(mData);

                break;
            }
            case ESM::fourCC("PSDT"): // reader.get(mSchedule); break;
            {
                if (subHdr.dataSize != sizeof(mSchedule))
                    reader.skipSubRecordData(); // FIXME:
                else
                    reader.get(mSchedule); // TES4

                break;
            }
            case ESM::fourCC("PLDT"):
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
            case ESM::fourCC("PTDT"):
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
            case ESM::fourCC("CTDA"):
            {
                if (subHdr.dataSize != sizeof(CTDA))
                {
                    reader.skipSubRecordData(); // FIXME: FO3
                    break;
                }

                CTDA condition;
                reader.get(condition);
                // FIXME: how to "unadjust" if not FormId?
                // adjustFormId(condition.param1);
                // adjustFormId(condition.param2);
                mConditions.push_back(condition);

                break;
            }
            case ESM::fourCC("CTDT"): // always 20 for TES4
            case ESM::fourCC("TNAM"): // FO3
            case ESM::fourCC("INAM"): // FO3
            case ESM::fourCC("CNAM"): // FO3
            case ESM::fourCC("SCHR"): // FO3
            case ESM::fourCC("POBA"): // FO3
            case ESM::fourCC("POCA"): // FO3
            case ESM::fourCC("POEA"): // FO3
            case ESM::fourCC("SCTX"): // FO3
            case ESM::fourCC("SCDA"): // FO3
            case ESM::fourCC("SCRO"): // FO3
            case ESM::fourCC("IDLA"): // FO3
            case ESM::fourCC("IDLC"): // FO3
            case ESM::fourCC("IDLF"): // FO3
            case ESM::fourCC("IDLT"): // FO3
            case ESM::fourCC("PKDD"): // FO3
            case ESM::fourCC("PKD2"): // FO3
            case ESM::fourCC("PKPT"): // FO3
            case ESM::fourCC("PKED"): // FO3
            case ESM::fourCC("PKE2"): // FO3
            case ESM::fourCC("PKAM"): // FO3
            case ESM::fourCC("PUID"): // FO3
            case ESM::fourCC("PKW3"): // FO3
            case ESM::fourCC("PTD2"): // FO3
            case ESM::fourCC("PLD2"): // FO3
            case ESM::fourCC("PKFD"): // FO3
            case ESM::fourCC("SLSD"): // FO3
            case ESM::fourCC("SCVR"): // FO3
            case ESM::fourCC("SCRV"): // FO3
            case ESM::fourCC("IDLB"): // FO3
            case ESM::fourCC("ANAM"): // TES5
            case ESM::fourCC("BNAM"): // TES5
            case ESM::fourCC("FNAM"): // TES5
            case ESM::fourCC("PNAM"): // TES5
            case ESM::fourCC("QNAM"): // TES5
            case ESM::fourCC("UNAM"): // TES5
            case ESM::fourCC("XNAM"): // TES5
            case ESM::fourCC("PDTO"): // TES5
            case ESM::fourCC("PTDA"): // TES5
            case ESM::fourCC("PFOR"): // TES5
            case ESM::fourCC("PFO2"): // TES5
            case ESM::fourCC("PRCB"): // TES5
            case ESM::fourCC("PKCU"): // TES5
            case ESM::fourCC("PKC2"): // TES5
            case ESM::fourCC("CITC"): // TES5
            case ESM::fourCC("CIS1"): // TES5
            case ESM::fourCC("CIS2"): // TES5
            case ESM::fourCC("VMAD"): // TES5
            case ESM::fourCC("TPIC"): // TES5
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::PACK::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::AIPackage::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::AIPackage::blank()
//{
// }
