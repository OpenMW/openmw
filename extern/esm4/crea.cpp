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
#include "crea.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream> // FIXME

#include "reader.hpp"
//#include "writer.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

ESM4::Creature::Creature() : mFormId(0), mFlags(0), mDeathItem(0), mScript(0), mCombatStyle(0),
                             mSoundBase(0), mSound(0), mSoundChance(0), mBaseScale(0.f),
                             mTurningSpeed(0.f), mFootWeight(0.f), mBoundRadius(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();

    mBloodSpray.clear();
    mBloodDecal.clear();

    mAIData.aggression = 0;
    mAIData.confidence = 0;
    mAIData.energyLevel = 0;
    mAIData.responsibility = 0;
    mAIData.aiFlags = 0;
    mAIData.trainSkill = 0;
    mAIData.trainLevel = 0;

    std::memset(&mData, 0, sizeof(Data));
}

ESM4::Creature::~Creature()
{
}

void ESM4::Creature::load(ESM4::Reader& reader)
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
            case ESM4::SUB_INAM: reader.getFormId(mDeathItem);   break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript);      break;
            case ESM4::SUB_AIDT:
            {
                if (subHdr.dataSize == 20) // FO3
                    reader.skipSubRecordData();
                else
                    reader.get(mAIData); // 12 bytes
                break;
            }
            case ESM4::SUB_ACBS:
            {
                if (subHdr.dataSize == 24) // FO3
                    reader.skipSubRecordData();
                else
                    reader.get(mBaseConfig);
                break;
            }
            case ESM4::SUB_DATA:
            {
                if (subHdr.dataSize == 17) // FO3
                    reader.skipSubRecordData();
                else
                    reader.get(mData);
                break;
            }
            case ESM4::SUB_ZNAM: reader.getFormId(mCombatStyle); break;
            case ESM4::SUB_CSCR: reader.getFormId(mSoundBase);   break;
            case ESM4::SUB_CSDI: reader.getFormId(mSound);       break;
            case ESM4::SUB_CSDC: reader.get(mSoundChance);  break;
            case ESM4::SUB_BNAM: reader.get(mBaseScale);    break;
            case ESM4::SUB_TNAM: reader.get(mTurningSpeed); break;
            case ESM4::SUB_WNAM: reader.get(mFootWeight);   break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_NAM0: reader.getZString(mBloodSpray); break;
            case ESM4::SUB_NAM1: reader.getZString(mBloodDecal); break;
            case ESM4::SUB_NIFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("CREA NIFZ data read error");

                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mNif.push_back(file);

                break;
            }
            case ESM4::SUB_NIFT:
            {
                if (subHdr.dataSize != 4) // FIXME: FO3
                {
                    reader.skipSubRecordData();
                    break;
                }

                assert(subHdr.dataSize == 4 && "CREA NIFT datasize error");
                std::uint32_t nift;
                reader.get(nift);
                if (nift)
                    std::cout << "CREA NIFT " << mFormId << ", non-zero " << nift << std::endl;
                break;
            }
            case ESM4::SUB_KFFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("CREA KFFZ data read error");

                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mKf.push_back(file);

                break;
            }
            case ESM4::SUB_MODT:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_CSDT:
            case ESM4::SUB_OBND: // FO3
            case ESM4::SUB_EAMT: // FO3
            case ESM4::SUB_VTCK: // FO3
            case ESM4::SUB_TPLT: // FO3
            case ESM4::SUB_PNAM: // FO3
            case ESM4::SUB_NAM4: // FO3
            case ESM4::SUB_NAM5: // FO3
            case ESM4::SUB_CNAM: // FO3
            case ESM4::SUB_LNAM: // FO3
            case ESM4::SUB_EITM: // FO3
            case ESM4::SUB_DEST: // FO3
            case ESM4::SUB_DSTD: // FO3
            case ESM4::SUB_DSTF: // FO3
            case ESM4::SUB_DMDL: // FO3
            case ESM4::SUB_DMDT: // FO3
            case ESM4::SUB_COED: // FO3
            {
                //std::cout << "CREA " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CREA::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Creature::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Creature::blank()
//{
//}
