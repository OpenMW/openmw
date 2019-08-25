/*
  Copyright (C) 2019 cc9cii

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
#include "bptd.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::BodyPart::BodyPart() : mFormId(0), mFlags(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mBPTName.clear();
    mNodeName.clear();
    mNodeTitle.clear();
    mNodeInfo.clear();

    std::memset(&mData, 0, sizeof(BPND));
}

ESM4::BodyPart::~BodyPart()
{
}

void ESM4::BodyPart::load(ESM4::Reader& reader)
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
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("BPTD FULL data read error");

                break;
            }
            case ESM4::SUB_BPND: reader.get(mData); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_BPTN:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mBPTName);
                else if (!reader.getZString(mBPTName))
                    throw std::runtime_error ("BPTD BPTN data read error");

                break;
            }
            case ESM4::SUB_BPNN: reader.getZString(mNodeName); break;
            case ESM4::SUB_BPNT: reader.getZString(mNodeTitle); break;
            case ESM4::SUB_BPNI: reader.getZString(mNodeInfo); break;
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODT:
            case ESM4::SUB_NAM1: // limb replacement model
            case ESM4::SUB_NAM4: // gore effects target bone
            case ESM4::SUB_NAM5:
            case ESM4::SUB_RAGA: // ragdoll
            {
                //std::cout << "BPTD " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::BPTD::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::BodyPart::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::BodyPart::blank()
//{
//}
