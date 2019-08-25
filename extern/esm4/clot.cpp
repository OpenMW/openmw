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
#include "clot.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Clothing::Clothing() : mFormId(0), mFlags(0), mBoundRadius(0.f), mClothingFlags(0),
                             mScript(0), mEnchantmentPoints(0), mEnchantment(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mIconMale.clear();
    mIconFemale.clear();

    mData.value = 0;
    mData.weight = 0.f;
}

ESM4::Clothing::~Clothing()
{
}

void ESM4::Clothing::load(ESM4::Reader& reader)
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
            case ESM4::SUB_FULL: reader.getZString(mFullName); break;
            case ESM4::SUB_MODL: reader.getZString(mModel);    break;
            case ESM4::SUB_ICON: reader.getZString(mIconMale); break;
            case ESM4::SUB_ICO2: reader.getZString(mIconFemale); break;
            case ESM4::SUB_DATA: reader.get(mData);            break;
            case ESM4::SUB_BMDT: reader.get(mClothingFlags);   break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript);      break;
            case ESM4::SUB_ENAM: reader.getFormId(mEnchantment); break;
            case ESM4::SUB_ANAM: reader.get(mEnchantmentPoints); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);     break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_MOD2:
            case ESM4::SUB_MOD3:
            case ESM4::SUB_MOD4:
            case ESM4::SUB_MO2B:
            case ESM4::SUB_MO3B:
            case ESM4::SUB_MO4B:
            case ESM4::SUB_MO2T:
            case ESM4::SUB_MO3T:
            case ESM4::SUB_MO4T:
            {
                //std::cout << "CLOT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CLOT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Clothing::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Clothing::blank()
//{
//}
