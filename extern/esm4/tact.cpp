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
#include "tact.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::TalkingActivator::TalkingActivator() : mFormId(0), mFlags(0), mScript(0), mVoiceType(0), mSound(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
}

ESM4::TalkingActivator::~TalkingActivator()
{
}

void ESM4::TalkingActivator::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("TACT FULL data read error");

                break;
            }
            case ESM4::SUB_SCRI: reader.getFormId(mScript); break;
            case ESM4::SUB_VNAM: reader.getFormId(mVoiceType); break;
            case ESM4::SUB_SNAM: reader.getFormId(mVoiceType); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_DEST: // destruction
            case ESM4::SUB_DSTD: // destruction
            case ESM4::SUB_DSTF: // destruction
            case ESM4::SUB_MODT: // texture file hash?
            case ESM4::SUB_FNAM:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_OBND:
            {
                //std::cout << "TACT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::TACT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::TalkingActivator::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::TalkingActivator::blank()
//{
//}
