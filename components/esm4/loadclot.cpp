/*
  Copyright (C) 2016, 2018, 2020 cc9cii

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
#include "loadclot.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Clothing::load(ESM4::Reader& reader)
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
                reader.getZString(mFullName);
                break;
            case ESM::fourCC("DATA"):
                reader.get(mData);
                break;
            case ESM::fourCC("BMDT"):
                reader.get(mClothingFlags);
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
                break;
            case ESM::fourCC("ENAM"):
                reader.getFormId(mEnchantment);
                break;
            case ESM::fourCC("ANAM"):
                reader.get(mEnchantmentPoints);
                break;
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("MODL"):
                reader.getZString(mModelMale);
                break;
            case ESM::fourCC("MOD2"):
                reader.getZString(mModelMaleWorld);
                break;
            case ESM::fourCC("MOD3"):
                reader.getZString(mModelFemale);
                break;
            case ESM::fourCC("MOD4"):
                reader.getZString(mModelFemaleWorld);
                break;
            case ESM::fourCC("ICON"):
                reader.getZString(mIconMale);
                break;
            case ESM::fourCC("ICO2"):
                reader.getZString(mIconFemale);
                break;
            case ESM::fourCC("MODT"):
            case ESM::fourCC("MO2B"):
            case ESM::fourCC("MO3B"):
            case ESM::fourCC("MO4B"):
            case ESM::fourCC("MO2T"):
            case ESM::fourCC("MO3T"):
            case ESM::fourCC("MO4T"):
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::CLOT::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
    // if ((mClothingFlags&0xffff) == 0x02) // only hair
    // std::cout << "only hair " << mEditorId << std::endl;
}

// void ESM4::Clothing::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Clothing::blank()
//{
// }
