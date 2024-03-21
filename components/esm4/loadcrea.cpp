/*
  Copyright (C) 2016, 2018, 2020-2021 cc9cii

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
#include "loadcrea.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

#include <components/debug/debuglog.hpp>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Creature::load(ESM4::Reader& reader)
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
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("CNTO"):
            {
                InventoryItem inv; // FIXME: use unique_ptr here?
                reader.get(inv);
                reader.adjustFormId(inv.item);
                mInventory.push_back(inv);
                break;
            }
            case ESM::fourCC("SPLO"):
                reader.getFormId(mSpell.emplace_back());
                break;
            case ESM::fourCC("PKID"):
                reader.getFormId(mAIPackages.emplace_back());
                break;
            case ESM::fourCC("SNAM"):
                reader.get(mFaction);
                reader.adjustFormId(mFaction.faction);
                break;
            case ESM::fourCC("INAM"):
                reader.getFormId(mDeathItem);
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
                break;
            case ESM::fourCC("AIDT"):
                if (subHdr.dataSize == 20) // FO3
                    reader.skipSubRecordData();
                else
                    reader.get(mAIData); // 12 bytes
                break;
            case ESM::fourCC("ACBS"):
                // if (esmVer == ESM::VER_094 || esmVer == ESM::VER_170 || mIsFONV)
                if (subHdr.dataSize == 24)
                    reader.get(mBaseConfig);
                else
                    reader.get(&mBaseConfig, 16); // TES4
                break;
            case ESM::fourCC("DATA"):
                if (subHdr.dataSize == 17) // FO3
                    reader.skipSubRecordData();
                else
                    reader.get(mData);
                break;
            case ESM::fourCC("ZNAM"):
                reader.getFormId(mCombatStyle);
                break;
            case ESM::fourCC("CSCR"):
                reader.getFormId(mSoundBase);
                break;
            case ESM::fourCC("CSDI"):
                reader.getFormId(mSound);
                break;
            case ESM::fourCC("CSDC"):
                reader.get(mSoundChance);
                break;
            case ESM::fourCC("BNAM"):
                reader.get(mBaseScale);
                break;
            case ESM::fourCC("TNAM"):
                reader.get(mTurningSpeed);
                break;
            case ESM::fourCC("WNAM"):
                reader.get(mFootWeight);
                break;
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("NAM0"):
                reader.getZString(mBloodSpray);
                break;
            case ESM::fourCC("NAM1"):
                reader.getZString(mBloodDecal);
                break;
            case ESM::fourCC("NIFZ"):
                if (!reader.getZeroTerminatedStringArray(mNif))
                    throw std::runtime_error("CREA NIFZ data read error");
                break;
            case ESM::fourCC("NIFT"):
            {
                if (subHdr.dataSize != 4) // FIXME: FO3
                {
                    reader.skipSubRecordData();
                    break;
                }

                if (subHdr.dataSize != 4)
                    throw std::runtime_error("CREA NIFT datasize error");
                std::uint32_t nift;
                reader.get(nift);
                if (nift)
                    Log(Debug::Verbose) << "CREA NIFT " << mId << ", non-zero " << nift;
                break;
            }
            case ESM::fourCC("KFFZ"):
                if (!reader.getZeroTerminatedStringArray(mKf))
                    throw std::runtime_error("CREA KFFZ data read error");
                break;
            case ESM::fourCC("TPLT"):
                reader.getFormId(mBaseTemplate);
                break; // FO3
            case ESM::fourCC("PNAM"): // FO3/FONV/TES5
                reader.getFormId(mBodyParts.emplace_back());
                break;
            case ESM::fourCC("MODT"):
            case ESM::fourCC("RNAM"):
            case ESM::fourCC("CSDT"):
            case ESM::fourCC("OBND"): // FO3
            case ESM::fourCC("EAMT"): // FO3
            case ESM::fourCC("VTCK"): // FO3
            case ESM::fourCC("NAM4"): // FO3
            case ESM::fourCC("NAM5"): // FO3
            case ESM::fourCC("CNAM"): // FO3
            case ESM::fourCC("LNAM"): // FO3
            case ESM::fourCC("EITM"): // FO3
            case ESM::fourCC("DEST"): // FO3
            case ESM::fourCC("DSTD"): // FO3
            case ESM::fourCC("DSTF"): // FO3
            case ESM::fourCC("DMDL"): // FO3
            case ESM::fourCC("DMDT"): // FO3
            case ESM::fourCC("COED"): // FO3
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::CREA::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Creature::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Creature::blank()
//{
// }
