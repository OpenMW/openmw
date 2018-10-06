/*
  Copyright (C) 2016-2018 cc9cii

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
#include "npc_.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Npc::Npc() : mFormId(0), mFlags(0), mRace(0), mClass(0), mHair(0), mEyes(0), mDeathItem(0),
                   mScript(0), mCombatStyle(0), mSoundBase(0), mSound(0), mSoundChance(0),
                   mFootWeight(0.f), mBoundRadius(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();

    std::memset(&mAIData, 0, sizeof(AIData));
    std::memset(&mData, 0, sizeof(Data));
    std::memset(&mBaseConfig, 0, sizeof(ActorBaseConfig));
    std::memset(&mFaction, 0, sizeof(ActorFaction));
}

ESM4::Npc::~Npc()
{
}

void ESM4::Npc::load(ESM4::Reader& reader)
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
            case ESM4::SUB_MODL: reader.getZString(mModel);    break;
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("NPC_ FULL data read error");

                break;
            }
            case ESM4::SUB_CNTO:
            {
                static InventoryItem inv; // FIXME: use unique_ptr here?
                reader.get(inv);
                reader.adjustFormId(inv.item);
                mInventory.push_back(inv);
                break;
            }
            case ESM4::SUB_SPLO:
            {
                FormId id;
                reader.getFormId(id);
                mSpell.push_back(id);
                break;
            }
            case ESM4::SUB_PKID:
            {
                FormId id;
                reader.getFormId(id);
                mAIPackages.push_back(id);
                break;
            }
            case ESM4::SUB_SNAM:
            {
                reader.get(mFaction);
                reader.adjustFormId(mFaction.faction);
                break;
            }
            case ESM4::SUB_RNAM: reader.getFormId(mRace);      break;
            case ESM4::SUB_CNAM: reader.getFormId(mClass);     break;
            case ESM4::SUB_HNAM: reader.getFormId(mHair);      break;
            case ESM4::SUB_ENAM: reader.getFormId(mEyes);      break;
            //
            case ESM4::SUB_INAM: reader.getFormId(mDeathItem); break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript);    break;
            //
            case ESM4::SUB_AIDT:
            {
                if (esmVer == ESM4::VER_094 || esmVer == ESM4::VER_170 || isFONV)
                {
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                    break;
                }

                reader.get(mAIData);
                break;
            }
            case ESM4::SUB_ACBS:
            {
                if (esmVer == ESM4::VER_094 || esmVer == ESM4::VER_170 || isFONV)
                {
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                    break;
                }

                reader.get(mBaseConfig);
                break;
            }
            case ESM4::SUB_DATA:
            {
                if (esmVer == ESM4::VER_094 || esmVer == ESM4::VER_170 || isFONV)
                {
                    if (subHdr.dataSize != 0) // FIXME FO3
                        reader.skipSubRecordData();
                    break; // zero length
                }

                reader.get(&mData, 33); // FIXME: check packing
                break;
            }
            case ESM4::SUB_ZNAM: reader.getFormId(mCombatStyle); break;
            case ESM4::SUB_CSCR: reader.getFormId(mSoundBase);   break;
            case ESM4::SUB_CSDI: reader.getFormId(mSound);       break;
            case ESM4::SUB_CSDC: reader.get(mSoundChance); break;
            case ESM4::SUB_WNAM: reader.get(mFootWeight);  break;
            //
            case ESM4::SUB_MODB: reader.get(mBoundRadius); break;
            case ESM4::SUB_KFFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("NPC_ KFFZ data read error");

                // Seems to be only below 3, and only happens 3 times while loading TES4:
                //   Forward_SheogorathWithCane.kf
                //   TurnLeft_SheogorathWithCane.kf
                //   TurnRight_SheogorathWithCane.kf
                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mKf.push_back(file);

                break;
            }
            case ESM4::SUB_LNAM:
            case ESM4::SUB_HCLR:
            case ESM4::SUB_FGGS:
            case ESM4::SUB_FGGA:
            case ESM4::SUB_FGTS:
            case ESM4::SUB_FNAM:
            case ESM4::SUB_ATKR:
            case ESM4::SUB_COCT:
            case ESM4::SUB_CRIF:
            case ESM4::SUB_CSDT:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_DOFT:
            case ESM4::SUB_DPLT:
            case ESM4::SUB_ECOR:
            case ESM4::SUB_ANAM:
            case ESM4::SUB_ATKD:
            case ESM4::SUB_ATKE:
            case ESM4::SUB_DEST:
            case ESM4::SUB_DSTD:
            case ESM4::SUB_DSTF:
            case ESM4::SUB_FTST:
            case ESM4::SUB_HCLF:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM5:
            case ESM4::SUB_NAM6:
            case ESM4::SUB_NAM7:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_NAMA:
            case ESM4::SUB_OBND:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_PRKR:
            case ESM4::SUB_PRKZ:
            case ESM4::SUB_QNAM:
            case ESM4::SUB_SOFT:
            case ESM4::SUB_SPCT:
            case ESM4::SUB_TIAS:
            case ESM4::SUB_TINC:
            case ESM4::SUB_TINI:
            case ESM4::SUB_TINV:
            case ESM4::SUB_TPLT:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_VTCK:
            case ESM4::SUB_GNAM:
            case ESM4::SUB_SHRT:
            case ESM4::SUB_SPOR:
            case ESM4::SUB_EAMT: // FO3
            case ESM4::SUB_NAM4: // FO3
            case ESM4::SUB_COED: // FO3
            {
                //std::cout << "NPC_ " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::NPC_::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Npc::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Npc::blank()
//{
//}
