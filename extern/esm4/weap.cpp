/*
  Copyright (C) 2016, 2018, 2019 cc9cii

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
#include "weap.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Weapon::Weapon() : mFormId(0), mFlags(0), mBoundRadius(0.f), mScript(0),
                         mEnchantmentPoints(0), mEnchantment(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mText.clear();
    mIcon.clear();
}

ESM4::Weapon::~Weapon()
{
}

void ESM4::Weapon::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();
    bool isFONV = esmVer == ESM4::VER_132 || esmVer == ESM4::VER_133 || esmVer == ESM4::VER_134;

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
                    throw std::runtime_error ("WEAP FULL data read error");

                break;
            }
            case ESM4::SUB_DATA:
            {
                //if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                if (subHdr.dataSize == 10) // FO3 has 15 bytes even though VER_094
                {
                    reader.get(mData.value);
                    reader.get(mData.weight);
                    reader.get(mData.damage);
                }
                else if (isFONV || subHdr.dataSize == 15)
                {
                    reader.get(mData.value);
                    reader.get(mData.health);
                    reader.get(mData.weight);
                    reader.get(mData.damage);
                    reader.get(mData.clipSize);
                }
                else
                {
                    reader.get(mData.type);
                    reader.get(mData.speed);
                    reader.get(mData.reach);
                    reader.get(mData.flags);
                    reader.get(mData.value);
                    reader.get(mData.health);
                    reader.get(mData.weight);
                    reader.get(mData.damage);
                }
                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_ICON: reader.getZString(mIcon);  break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript); break;
            case ESM4::SUB_ANAM: reader.get(mEnchantmentPoints); break;
            case ESM4::SUB_ENAM: reader.getFormId(mEnchantment); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_DESC:
            {
                if (reader.hasLocalizedStrings())
                {
                    std::uint32_t formid;
                    reader.get(formid);
                    if (formid)
                        reader.getLocalizedString(formid, mText);
                }
                else if (!reader.getZString(mText))
                    throw std::runtime_error ("WEAP DESC data read error");

                break;
            }
            case ESM4::SUB_MODT:
            case ESM4::SUB_BAMT:
            case ESM4::SUB_BIDS:
            case ESM4::SUB_INAM:
            case ESM4::SUB_CNAM:
            case ESM4::SUB_CRDT:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_EAMT:
            case ESM4::SUB_EITM:
            case ESM4::SUB_ETYP:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_OBND:
            case ESM4::SUB_SNAM:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_UNAM:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_VNAM:
            case ESM4::SUB_WNAM:
            case ESM4::SUB_XNAM: // Dawnguard only?
            case ESM4::SUB_NNAM:
            case ESM4::SUB_MODS:
            case ESM4::SUB_NAM0: // FO3
            case ESM4::SUB_MICO: // FO3
            case ESM4::SUB_REPL: // FO3
            case ESM4::SUB_YNAM: // FO3
            case ESM4::SUB_ZNAM: // FO3
            case ESM4::SUB_MOD2: // FO3
            case ESM4::SUB_MO2T: // FO3
            case ESM4::SUB_MO2S: // FO3
            case ESM4::SUB_NAM6: // FO3
            case ESM4::SUB_MOD4: // FO3
            case ESM4::SUB_MO4T: // FO3
            case ESM4::SUB_MO4S: // FO3
            case ESM4::SUB_BIPL: // FO3
            case ESM4::SUB_NAM7: // FO3
            case ESM4::SUB_MOD3: // FO3
            case ESM4::SUB_MO3T: // FO3
            case ESM4::SUB_MO3S: // FO3
            case ESM4::SUB_MODD: // FO3
          //case ESM4::SUB_MOSD: // FO3
            case ESM4::SUB_DEST: // FO3
            case ESM4::SUB_DSTD: // FO3
            case ESM4::SUB_DSTF: // FO3
            case ESM4::SUB_DMDL: // FO3
            case ESM4::SUB_DMDT: // FO3
            case ESM4::SUB_VATS: // FONV
            case ESM4::SUB_VANM: // FONV
            case ESM4::SUB_MWD1: // FONV
            case ESM4::SUB_MWD2: // FONV
            case ESM4::SUB_MWD3: // FONV
            case ESM4::SUB_MWD4: // FONV
            case ESM4::SUB_MWD5: // FONV
            case ESM4::SUB_MWD6: // FONV
            case ESM4::SUB_MWD7: // FONV
            case ESM4::SUB_WMI1: // FONV
            case ESM4::SUB_WMI2: // FONV
            case ESM4::SUB_WMI3: // FONV
            case ESM4::SUB_WMS1: // FONV
            case ESM4::SUB_WMS2: // FONV
            case ESM4::SUB_WNM1: // FONV
            case ESM4::SUB_WNM2: // FONV
            case ESM4::SUB_WNM3: // FONV
            case ESM4::SUB_WNM4: // FONV
            case ESM4::SUB_WNM5: // FONV
            case ESM4::SUB_WNM6: // FONV
            case ESM4::SUB_WNM7: // FONV
            case ESM4::SUB_EFSD: // FONV DeadMoney
            {
                //std::cout << "WEAP " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::WEAP::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Weapon::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Weapon::blank()
//{
//}
