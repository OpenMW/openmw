/*
  Copyright (C) 2016-2021 cc9cii

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
#include "loadnpc.hpp"

#include <cstring>
#include <stdexcept>
#include <string> // getline

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Npc::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    std::uint32_t esmVer = reader.esmVersion();
    mIsTES4 = (esmVer == ESM::VER_080 || esmVer == ESM::VER_100) && !reader.hasFormVersion();
    mIsFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;
    // mIsTES5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_170; // WARN: FO3 is also VER_094

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break; // not for TES5, see Race
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
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
            {
                // FO4, FO76
                if (subHdr.dataSize == 5)
                    reader.get(&mFaction, 5);
                else
                    reader.get(mFaction);
                reader.adjustFormId(mFaction.faction);
                break;
            }
            case ESM::fourCC("RNAM"):
                reader.getFormId(mRace);
                break;
            case ESM::fourCC("CNAM"):
                reader.getFormId(mClass);
                break;
            case ESM::fourCC("HNAM"):
                reader.getFormId(mHair);
                break; // not for TES5
            case ESM::fourCC("ENAM"):
                reader.getFormId(mEyes);
                break;
            //
            case ESM::fourCC("INAM"):
                reader.getFormId(mDeathItem);
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
                break;
            //
            case ESM::fourCC("AIDT"):
            {
                if (subHdr.dataSize != 12)
                {
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                    break;
                }

                reader.get(mAIData); // TES4
                break;
            }
            case ESM::fourCC("ACBS"):
            {
                switch (subHdr.dataSize)
                {
                    case 20: // FO4
                        mIsFO4 = true;
                        [[fallthrough]];
                    case 16: // TES4
                    case 24: // FO3/FNV, TES5
                        reader.get(&mBaseConfig, subHdr.dataSize);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM::fourCC("DATA"):
            {
                if (subHdr.dataSize == 0)
                    break;

                if (subHdr.dataSize == 33)
                    reader.get(&mData, 33); // FIXME: check packing
                else // FIXME FO3
                    reader.skipSubRecordData();
                break;
            }
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
            case ESM::fourCC("WNAM"):
            {
                // FIXME: should be read into mWornArmor for FO4
                if (reader.esmVersion() == ESM::VER_094 || reader.esmVersion() == ESM::VER_170)
                    reader.getFormId(mWornArmor);
                else
                    reader.get(mFootWeight);
                break;
            }
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("KFFZ"):
            {
                // Seems to be only below 3, and only happens 3 times while loading TES4:
                //   Forward_SheogorathWithCane.kf
                //   TurnLeft_SheogorathWithCane.kf
                //   TurnRight_SheogorathWithCane.kf
                if (!reader.getZeroTerminatedStringArray(mKf))
                    throw std::runtime_error("NPC_ KFFZ data read error");
                break;
            }
            case ESM::fourCC("LNAM"):
                reader.get(mHairLength);
                break;
            case ESM::fourCC("HCLR"):
            {
                reader.get(mHairColour.red);
                reader.get(mHairColour.green);
                reader.get(mHairColour.blue);
                reader.get(mHairColour.custom);

                break;
            }
            case ESM::fourCC("TPLT"):
                reader.getFormId(mBaseTemplate);
                break;
            case ESM::fourCC("FGGS"):
            {
                mSymShapeModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymShapeModeCoefficients.at(i));

                break;
            }
            case ESM::fourCC("FGGA"):
            {
                mAsymShapeModeCoefficients.resize(30);
                for (std::size_t i = 0; i < 30; ++i)
                    reader.get(mAsymShapeModeCoefficients.at(i));

                break;
            }
            case ESM::fourCC("FGTS"):
            {
                mSymTextureModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymTextureModeCoefficients.at(i));

                break;
            }
            case ESM::fourCC("FNAM"):
            {
                reader.get(mFgRace);
                // std::cout << "race " << mEditorId << " " << mRace << std::endl; // FIXME
                // std::cout << "fg race " << mEditorId << " " << mFgRace << std::endl; // FIXME
                break;
            }
            case ESM::fourCC("PNAM"): // FO3/FONV/TES5
                reader.getFormId(mHeadParts.emplace_back());
                break;
            case ESM::fourCC("HCLF"): // TES5 hair colour
            {
                reader.getFormId(mHairColourId);

                break;
            }
            case ESM::fourCC("BCLF"):
            {
                reader.getFormId(mBeardColourId);
                break;
            }
            case ESM::fourCC("COCT"): // TES5
            {
                std::uint32_t count;
                reader.get(count);

                break;
            }
            case ESM::fourCC("DOFT"):
                reader.getFormId(mDefaultOutfit);
                break;
            case ESM::fourCC("SOFT"):
                reader.getFormId(mSleepOutfit);
                break;
            case ESM::fourCC("DPLT"):
                reader.getFormId(mDefaultPkg);
                break; // AI package list
            case ESM::fourCC("DAMC"): // Destructible
            case ESM::fourCC("DEST"):
            case ESM::fourCC("DMDC"):
            case ESM::fourCC("DMDL"):
            case ESM::fourCC("DMDT"):
            case ESM::fourCC("DMDS"):
            case ESM::fourCC("DSTA"):
            case ESM::fourCC("DSTD"):
            case ESM::fourCC("DSTF"): // Destructible end
            case ESM::fourCC("NAM6"): // height mult
            case ESM::fourCC("NAM7"): // weight mult
            case ESM::fourCC("ATKR"):
            case ESM::fourCC("CRIF"):
            case ESM::fourCC("CSDT"):
            case ESM::fourCC("DNAM"):
            case ESM::fourCC("ECOR"):
            case ESM::fourCC("ANAM"):
            case ESM::fourCC("ATKD"):
            case ESM::fourCC("ATKE"):
            case ESM::fourCC("FTST"):
            case ESM::fourCC("KSIZ"):
            case ESM::fourCC("KWDA"):
            case ESM::fourCC("NAM5"):
            case ESM::fourCC("NAM8"):
            case ESM::fourCC("NAM9"):
            case ESM::fourCC("NAMA"):
            case ESM::fourCC("OBND"):
            case ESM::fourCC("PRKR"):
            case ESM::fourCC("PRKZ"):
            case ESM::fourCC("QNAM"):
            case ESM::fourCC("SPCT"):
            case ESM::fourCC("TIAS"):
            case ESM::fourCC("TINC"):
            case ESM::fourCC("TINI"):
            case ESM::fourCC("TINV"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("VTCK"):
            case ESM::fourCC("GNAM"):
            case ESM::fourCC("SHRT"):
            case ESM::fourCC("SPOR"):
            case ESM::fourCC("EAMT"): // FO3
            case ESM::fourCC("NAM4"): // FO3
            case ESM::fourCC("COED"): // FO3
            case ESM::fourCC("APPR"): // FO4
            case ESM::fourCC("ATKS"): // FO4
            case ESM::fourCC("ATKT"): // FO4
            case ESM::fourCC("ATKW"): // FO4
            case ESM::fourCC("ATTX"): // FO4
            case ESM::fourCC("FTYP"): // FO4
            case ESM::fourCC("LTPT"): // FO4
            case ESM::fourCC("LTPC"): // FO4
            case ESM::fourCC("MWGT"): // FO4
            case ESM::fourCC("NTRM"): // FO4
            case ESM::fourCC("PFRN"): // FO4
            case ESM::fourCC("PRPS"): // FO4
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("STCP"): // FO4
            case ESM::fourCC("TETI"): // FO4
            case ESM::fourCC("TEND"): // FO4
            case ESM::fourCC("TPTA"): // FO4
            case ESM::fourCC("OBTE"): // FO4 object template start
            case ESM::fourCC("OBTF"): //
            case ESM::fourCC("OBTS"): //
            case ESM::fourCC("STOP"): // FO4 object template end
            case ESM::fourCC("OCOR"): // FO4 new package lists start
            case ESM::fourCC("GWOR"): //
            case ESM::fourCC("FCPL"): //
            case ESM::fourCC("RCLR"): // FO4 new package lists end
            case ESM::fourCC("CS2D"): // FO4 actor sound subrecords
            case ESM::fourCC("CS2E"): //
            case ESM::fourCC("CS2F"): //
            case ESM::fourCC("CS2H"): //
            case ESM::fourCC("CS2K"): // FO4 actor sound subrecords end
            case ESM::fourCC("MSDK"): // FO4 morph subrecords start
            case ESM::fourCC("MSDV"): //
            case ESM::fourCC("MRSV"): //
            case ESM::fourCC("FMRI"): //
            case ESM::fourCC("FMRS"): //
            case ESM::fourCC("FMIN"): // FO4 morph subrecords end
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::NPC_::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Npc::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Npc::blank()
//{
// }
