/*
  Copyright (C) 2019-2021 cc9cii

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
#include "loadbptd.hpp"

#include <stdexcept>
#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::BodyPartData::BodyPart::clear()
{
    mPartName.clear();
    mNodeName.clear();
    mVATSTarget.clear();
    mIKStartNode.clear();
    std::memset(&mData, 0, sizeof(BPND));
    mLimbReplacementModel.clear();
    mGoreEffectsTarget.clear();
}

void ESM4::BodyPartData::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    BodyPart bodyPart;
    bodyPart.clear();

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_FULL: reader.getLocalizedString(mFullName); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_BPTN: reader.getLocalizedString(bodyPart.mPartName); break;
            case ESM4::SUB_BPNN: reader.getZString(bodyPart.mNodeName); break;
            case ESM4::SUB_BPNT: reader.getZString(bodyPart.mVATSTarget); break;
            case ESM4::SUB_BPNI: reader.getZString(bodyPart.mIKStartNode); break;
            case ESM4::SUB_BPND: reader.get(bodyPart.mData); break;
            case ESM4::SUB_NAM1: reader.getZString(bodyPart.mLimbReplacementModel); break;
            case ESM4::SUB_NAM4: // FIXME: assumed occurs last
            {
                reader.getZString(bodyPart.mGoreEffectsTarget); // target bone

                mBodyParts.push_back(bodyPart); // FIXME: possible without copying?

                bodyPart.clear();
                break;
            }
            case ESM4::SUB_NAM5:
            case ESM4::SUB_RAGA: // ragdoll
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODT:
            {
                //std::cout << "BPTD " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::BPTD::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }

    //if (mEditorId == "DefaultBodyPartData")
        //std::cout << "BPTD: " << mEditorId << std::endl; // FIXME: testing only
}

//void ESM4::BodyPartData::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::BodyPartData::blank()
//{
//}
