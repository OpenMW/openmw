/*
  Copyright (C) 2016, 2018-2021 cc9cii

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
#include "loadammo.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Ammunition::load(ESM4::Reader& reader)
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
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("DATA"):
                switch (subHdr.dataSize)
                {
                    case 18: // TES4
                    case 13: // FO3/FNV
                    {
                        reader.get(mData.mSpeed);
                        reader.get(mData.mFlags);
                        mData.mFlags &= 0xFF;
                        reader.get(mData.mValue);
                        if (subHdr.dataSize == 13)
                            reader.get(mData.mClipRounds);
                        else
                        {
                            reader.get(mData.mWeight);
                            std::uint16_t damageInt;
                            reader.get(damageInt);
                            mData.mDamage = static_cast<float>(damageInt);
                        }
                        break;
                    }
                    case 16: // TES5
                    case 20: // SSE
                        reader.getFormId(mData.mProjectile);
                        reader.get(mData.mFlags);
                        reader.get(mData.mDamage);
                        reader.get(mData.mValue);
                        if (subHdr.dataSize == 20)
                            reader.get(mData.mWeight);
                        break;
                    case 8:
                        reader.get(mData.mValue);
                        reader.get(mData.mWeight);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            case ESM::fourCC("DAT2"):
                if (subHdr.dataSize == 20)
                {
                    reader.get(mData.mProjPerShot);
                    reader.getFormId(mData.mProjectile);
                    reader.get(mData.mWeight);
                    reader.getFormId(mData.mConsumedAmmo);
                    reader.get(mData.mConsumedPercentage);
                }
                else
                {
                    reader.skipSubRecordData();
                }
                break;
            case ESM::fourCC("DNAM"):
                reader.getFormId(mData.mProjectile);
                reader.get(mData.mFlags);
                mData.mFlags &= 0xFF;
                reader.get(mData.mDamage);
                reader.get(mData.mHealth);
                break;
            case ESM::fourCC("ICON"):
                reader.getZString(mIcon);
                break;
            case ESM::fourCC("MICO"):
                reader.getZString(mMiniIcon);
                break; // FO3
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("ANAM"):
                reader.get(mEnchantmentPoints);
                break;
            case ESM::fourCC("ENAM"):
                reader.getFormId(mEnchantment);
                break;
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("DESC"):
                reader.getLocalizedString(mText);
                break;
            case ESM::fourCC("YNAM"):
                reader.getFormId(mPickUpSound);
                break;
            case ESM::fourCC("ZNAM"):
                reader.getFormId(mDropSound);
                break;
            case ESM::fourCC("ONAM"):
                reader.getLocalizedString(mShortName);
                break;
            case ESM::fourCC("QNAM"): // FONV
                reader.getLocalizedString(mAbbrev);
                break;
            case ESM::fourCC("RCIL"):
                reader.getFormId(mAmmoEffects.emplace_back());
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScript);
                break;
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("OBND"):
            case ESM::fourCC("KSIZ"):
            case ESM::fourCC("KWDA"):
            case ESM::fourCC("DAMC"): // Destructible
            case ESM::fourCC("DEST"):
            case ESM::fourCC("DMDC"):
            case ESM::fourCC("DMDL"):
            case ESM::fourCC("DMDT"):
            case ESM::fourCC("DMDS"):
            case ESM::fourCC("DSTA"):
            case ESM::fourCC("DSTD"):
            case ESM::fourCC("DSTF"): // Destructible end
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("NAM1"): // FO4 casing model data
            case ESM::fourCC("NAM2"): //
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::AMMO::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Ammunition::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Ammunition::blank()
//{
// }
