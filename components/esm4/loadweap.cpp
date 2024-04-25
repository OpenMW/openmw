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
#include "loadweap.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Weapon::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();
    bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

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
            {
                // if (reader.esmVersion() == ESM::VER_094 || reader.esmVersion() == ESM::VER_170)
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
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("ICON"):
                reader.getZString(mIcon);
                break;
            case ESM::fourCC("MICO"):
                reader.getZString(mMiniIcon);
                break; // FO3
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
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
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("BAMT"):
            case ESM::fourCC("BIDS"):
            case ESM::fourCC("INAM"):
            case ESM::fourCC("CNAM"):
            case ESM::fourCC("CRDT"):
            case ESM::fourCC("DNAM"):
            case ESM::fourCC("EAMT"):
            case ESM::fourCC("EITM"):
            case ESM::fourCC("ETYP"):
            case ESM::fourCC("KSIZ"):
            case ESM::fourCC("KWDA"):
            case ESM::fourCC("NAM8"):
            case ESM::fourCC("NAM9"):
            case ESM::fourCC("OBND"):
            case ESM::fourCC("SNAM"):
            case ESM::fourCC("TNAM"):
            case ESM::fourCC("UNAM"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("VNAM"):
            case ESM::fourCC("WNAM"):
            case ESM::fourCC("XNAM"): // Dawnguard only?
            case ESM::fourCC("NNAM"):
            case ESM::fourCC("NAM0"): // FO3
            case ESM::fourCC("REPL"): // FO3
            case ESM::fourCC("MOD2"): // FO3
            case ESM::fourCC("MO2T"): // FO3
            case ESM::fourCC("MO2S"): // FO3
            case ESM::fourCC("NAM6"): // FO3
            case ESM::fourCC("MOD4"): // First person model data
            case ESM::fourCC("MO4T"):
            case ESM::fourCC("MO4S"):
            case ESM::fourCC("MO4C"):
            case ESM::fourCC("MO4F"): // First person model data end
            case ESM::fourCC("BIPL"): // FO3
            case ESM::fourCC("NAM7"): // FO3
            case ESM::fourCC("MOD3"): // FO3
            case ESM::fourCC("MO3T"): // FO3
            case ESM::fourCC("MO3S"): // FO3
            case ESM::fourCC("MODD"): // FO3
                                      // case ESM::fourCC("MOSD"): // FO3
            case ESM::fourCC("DAMC"): // Destructible
            case ESM::fourCC("DEST"):
            case ESM::fourCC("DMDC"):
            case ESM::fourCC("DMDL"):
            case ESM::fourCC("DMDT"):
            case ESM::fourCC("DMDS"):
            case ESM::fourCC("DSTA"):
            case ESM::fourCC("DSTD"):
            case ESM::fourCC("DSTF"): // Destructible end
            case ESM::fourCC("VATS"): // FONV
            case ESM::fourCC("VANM"): // FONV
            case ESM::fourCC("MWD1"): // FONV
            case ESM::fourCC("MWD2"): // FONV
            case ESM::fourCC("MWD3"): // FONV
            case ESM::fourCC("MWD4"): // FONV
            case ESM::fourCC("MWD5"): // FONV
            case ESM::fourCC("MWD6"): // FONV
            case ESM::fourCC("MWD7"): // FONV
            case ESM::fourCC("WMI1"): // FONV
            case ESM::fourCC("WMI2"): // FONV
            case ESM::fourCC("WMI3"): // FONV
            case ESM::fourCC("WMS1"): // FONV
            case ESM::fourCC("WMS2"): // FONV
            case ESM::fourCC("WNM1"): // FONV
            case ESM::fourCC("WNM2"): // FONV
            case ESM::fourCC("WNM3"): // FONV
            case ESM::fourCC("WNM4"): // FONV
            case ESM::fourCC("WNM5"): // FONV
            case ESM::fourCC("WNM6"): // FONV
            case ESM::fourCC("WNM7"): // FONV
            case ESM::fourCC("EFSD"): // FONV DeadMoney
            case ESM::fourCC("APPR"): // FO4
            case ESM::fourCC("DAMA"): // FO4
            case ESM::fourCC("FLTR"): // FO4
            case ESM::fourCC("FNAM"): // FO4
            case ESM::fourCC("INRD"): // FO4
            case ESM::fourCC("LNAM"): // FO4
            case ESM::fourCC("MASE"): // FO4
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("STCP"): // FO4
            case ESM::fourCC("WAMD"): // FO4
            case ESM::fourCC("WZMD"): // FO4
            case ESM::fourCC("OBTE"): // FO4 object template start
            case ESM::fourCC("OBTF"):
            case ESM::fourCC("OBTS"):
            case ESM::fourCC("STOP"): // FO4 object template end
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::WEAP::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Weapon::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Weapon::blank()
//{
// }
