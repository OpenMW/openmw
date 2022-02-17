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
#include "loaddial.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Dialogue::Dialogue() : mFormId(0), mFlags(0), mDoAllBeforeRepeat(false),
                             mDialType(0), mDialFlags(0), mPriority(0.f)
{
    mEditorId.clear();
    mTopicName.clear();

    mTextDumb.clear(); // FIXME: temp name
}

ESM4::Dialogue::~Dialogue()
{
}

void ESM4::Dialogue::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId);  break;
            case ESM4::SUB_FULL: reader.getZString(mTopicName); break;
            case ESM4::SUB_QSTI:
            {
                FormId questId;
                reader.getFormId(questId);
                mQuests.push_back(questId);

                break;
            }
            case ESM4::SUB_QSTR: // Seems never used in TES4
            {
                FormId questRem;
                reader.getFormId(questRem);
                mQuestsRemoved.push_back(questRem);

                break;
            }
            case ESM4::SUB_DATA:
            {
                if (subHdr.dataSize == 4) // TES5
                {
                    std::uint8_t dummy;
                    reader.get(dummy);
                    if (dummy != 0)
                        mDoAllBeforeRepeat = true;
                }

                reader.get(mDialType); // TES4/FO3/FONV/TES5

                if (subHdr.dataSize >= 2) // FO3/FONV/TES5
                    reader.get(mDialFlags);

                if (subHdr.dataSize >= 3) // TES5
                    reader.skipSubRecordData(1); // unknown

                break;
            }
            case ESM4::SUB_PNAM: reader.get(mPriority); break; // FO3/FONV
            case ESM4::SUB_TDUM: reader.getZString(mTextDumb); break; // FONV
            case ESM4::SUB_SCRI:
            case ESM4::SUB_INFC: // FONV info connection
            case ESM4::SUB_INFX: // FONV info index
            case ESM4::SUB_QNAM: // TES5
            case ESM4::SUB_BNAM: // TES5
            case ESM4::SUB_SNAM: // TES5
            case ESM4::SUB_TIFC: // TES5
            {
                //std::cout << "DIAL " << ESM::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::DIAL::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Dialogue::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Dialogue::blank()
//{
//}
