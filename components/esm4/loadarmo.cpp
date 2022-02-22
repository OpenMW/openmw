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
#include "loadarmo.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Armor::~Armor()
{
}

void ESM4::Armor::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();
    mIsFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_FULL: reader.getLocalizedString(mFullName); break;
            case ESM4::SUB_DATA:
            {
                //if (reader.esmVersion() == ESM::VER_094 || reader.esmVersion() == ESM::VER_170)
                if (subHdr.dataSize == 8) // FO3 has 12 bytes even though VER_094
                {
                    reader.get(mData.value);
                    reader.get(mData.weight);
                    mIsFO3 = true;
                }
                else if (mIsFONV || subHdr.dataSize == 12)
                {
                    reader.get(mData.value);
                    reader.get(mData.health);
                    reader.get(mData.weight);
                }
                else
                {
                    reader.get(mData); // TES4
                    mIsTES4 = true;
                }

                break;
            }
            case ESM4::SUB_MODL: // seems only for Dawnguard/Dragonborn?
            {
                //if (esmVer == ESM::VER_094 || esmVer == ESM::VER_170 || isFONV)
                if (subHdr.dataSize == 4) // FO3 has zstring even though VER_094
                {
                    FormId formId;
                    reader.getFormId(formId);

                    mAddOns.push_back(formId);
                }
                else
                {
                    if (!reader.getZString(mModelMale))
                        throw std::runtime_error ("ARMO MODL data read error");
                }

                break;
            }
            case ESM4::SUB_MOD2: reader.getZString(mModelMaleWorld);break;
            case ESM4::SUB_MOD3: reader.getZString(mModelFemale); break;
            case ESM4::SUB_MOD4: reader.getZString(mModelFemaleWorld); break;
            case ESM4::SUB_ICON: reader.getZString(mIconMale);   break;
            case ESM4::SUB_MICO: reader.getZString(mMiniIconMale);   break;
            case ESM4::SUB_ICO2: reader.getZString(mIconFemale); break;
            case ESM4::SUB_MIC2: reader.getZString(mMiniIconFemale); break;
            case ESM4::SUB_BMDT:
            {
                if (subHdr.dataSize == 8) // FO3
                {
                    reader.get(mArmorFlags);
                    reader.get(mGeneralFlags);
                    mGeneralFlags &= 0x000000ff;
                    mGeneralFlags |= TYPE_FO3;
                }
                else                      // TES4
                {
                    reader.get(mArmorFlags);
                    mGeneralFlags = (mArmorFlags & 0x00ff0000) >> 16;
                    mGeneralFlags |= TYPE_TES4;
                }
                break;
            }
            case ESM4::SUB_BODT:
            {
                reader.get(mArmorFlags);
                uint32_t flags = 0;
                if (subHdr.dataSize == 12)
                    reader.get(flags);
                reader.get(mGeneralFlags);   // skill
                mGeneralFlags &= 0x0000000f; // 0 (light), 1 (heavy) or 2 (none)
                if (subHdr.dataSize == 12)
                    mGeneralFlags |= (flags & 0x0000000f) << 3;
                mGeneralFlags |= TYPE_TES5;
                break;
            }
            case ESM4::SUB_BOD2:
            {
                reader.get(mArmorFlags);
                reader.get(mGeneralFlags);
                mGeneralFlags &= 0x0000000f; // 0 (light), 1 (heavy) or 2 (none)
                mGeneralFlags |= TYPE_TES5;
                break;
            }
            case ESM4::SUB_SCRI: reader.getFormId(mScriptId);      break;
            case ESM4::SUB_ANAM: reader.get(mEnchantmentPoints); break;
            case ESM4::SUB_ENAM: reader.getFormId(mEnchantment); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);       break;
            case ESM4::SUB_DESC: reader.getLocalizedString(mText); break;
            case ESM4::SUB_YNAM: reader.getFormId(mPickUpSound); break;
            case ESM4::SUB_ZNAM: reader.getFormId(mDropSound); break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_MO2B:
            case ESM4::SUB_MO3B:
            case ESM4::SUB_MO4B:
            case ESM4::SUB_MO2T:
            case ESM4::SUB_MO2S:
            case ESM4::SUB_MO3T:
            case ESM4::SUB_MO4T:
            case ESM4::SUB_MO4S:
            case ESM4::SUB_OBND:
            case ESM4::SUB_RNAM: // race formid
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_BAMT:
            case ESM4::SUB_BIDS:
            case ESM4::SUB_ETYP:
            case ESM4::SUB_BMCT:
            case ESM4::SUB_EAMT:
            case ESM4::SUB_EITM:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_REPL: // FO3
            case ESM4::SUB_BIPL: // FO3
            case ESM4::SUB_MODD: // FO3
            case ESM4::SUB_MOSD: // FO3
            case ESM4::SUB_MODS: // FO3
            case ESM4::SUB_MO3S: // FO3
            case ESM4::SUB_BNAM: // FONV
            case ESM4::SUB_SNAM: // FONV
            {
                //std::cout << "ARMO " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::ARMO::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
    //if ((mArmorFlags&0xffff) == 0x02) // only hair
        //std::cout << "only hair " << mEditorId << std::endl;
}

//void ESM4::Armor::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Armor::blank()
//{
//}
