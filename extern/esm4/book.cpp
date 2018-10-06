/*
  Copyright (C) 2016, 2018 cc9cii

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
#include "book.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Book::Book() : mFormId(0), mFlags(0), mBoundRadius(0.f), mScript(0),
                     mEnchantmentPoints(0), mEnchantment(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mText.clear();
    mIcon.clear();

    mData.flags = 0;
    mData.type = 0;
    mData.bookSkill = 0;
    mData.value = 0;
    mData.weight = 0.f;
}

ESM4::Book::~Book()
{
}

void ESM4::Book::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();

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
                    throw std::runtime_error ("BOOK FULL data read error");

                break;
            }
            case ESM4::SUB_DESC:
            {
                if (reader.hasLocalizedStrings())
                {
                    std::uint32_t formid;
                    reader.get(formid);
                    if (formid)
                        reader.getLocalizedString(formid, mText); // sometimes formid is null
                }
                else if (!reader.getZString(mText))
                    throw std::runtime_error ("BOOK DESC data read error");

                break;
            }
            case ESM4::SUB_DATA:
            {
                reader.get(mData.flags);
                //if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                if (subHdr.dataSize == 16) // FO3 has 10 bytes even though VER_094
                {
                    static std::uint8_t dummy;
                    reader.get(mData.type);
                    reader.get(dummy);
                    reader.get(dummy);
                    reader.get(mData.teaches);
                }
                else
                {
                    reader.get(mData.bookSkill);
                }
                reader.get(mData.value);
                reader.get(mData.weight);
                break;
            }
            case ESM4::SUB_ICON: reader.getZString(mIcon);  break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript);      break;
            case ESM4::SUB_ANAM: reader.get(mEnchantmentPoints); break;
            case ESM4::SUB_ENAM: reader.getFormId(mEnchantment); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_OBND:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_CNAM:
            case ESM4::SUB_INAM:
            case ESM4::SUB_YNAM:
            case ESM4::SUB_VMAD:
            {
                //std::cout << "BOOK " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::BOOK::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Book::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Book::blank()
//{
//}
