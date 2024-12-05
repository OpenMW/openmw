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

#include <cstring>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Dialogue::load(ESM4::Reader& reader)
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
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mTopicName);
                break;
            case ESM::fourCC("QSTI"):
                reader.getFormId(mQuests.emplace_back());
                break;
            case ESM::fourCC("QSTR"): // Seems never used in TES4
                reader.getFormId(mQuestsRemoved.emplace_back());
                break;
            case ESM::fourCC("DATA"):
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
            case ESM::fourCC("PNAM"):
                reader.get(mPriority);
                break; // FO3/FONV
            case ESM::fourCC("TDUM"):
                reader.getZString(mTextDumb);
                break; // FONV
            case ESM::fourCC("SCRI"):
            case ESM::fourCC("INFC"): // FONV info connection
            case ESM::fourCC("INFX"): // FONV info index
            case ESM::fourCC("QNAM"): // TES5
            case ESM::fourCC("BNAM"): // TES5
            case ESM::fourCC("SNAM"): // TES5
            case ESM::fourCC("TIFC"): // TES5
            case ESM::fourCC("KNAM"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::DIAL::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Dialogue::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Dialogue::blank()
//{
// }
