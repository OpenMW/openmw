#include "loadcrea.hpp"

#include <components/debug/debuglog.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void Creature::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mAiPackage.mList.clear();
        mInventory.mList.clear();
        mSpells.mList.clear();
        mTransport.mList.clear();

        mScale = 1.f;
        mAiData.blank();
        mAiData.mFight = 90;
        mAiData.mFlee = 20;

        bool hasName = false;
        bool hasNpdt = false;
        bool hasFlags = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("CNAM"):
                    mOriginal = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getHString();
                    break;
                case fourCC("NPDT"):
                    esm.getHTSized<96>(mData);
                    hasNpdt = true;
                    break;
                case fourCC("FLAG"):
                    int flags;
                    esm.getHT(flags);
                    mFlags = flags & 0xFF;
                    mBloodType = ((flags >> 8) & 0xFF) >> 2;
                    hasFlags = true;
                    break;
                case fourCC("XSCL"):
                    esm.getHT(mScale);
                    break;
                case fourCC("NPCO"):
                    mInventory.add(esm);
                    break;
                case fourCC("NPCS"):
                    mSpells.add(esm);
                    break;
                case fourCC("AIDT"):
                    esm.getHExact(&mAiData, sizeof(mAiData));
                    break;
                case fourCC("DODT"):
                case fourCC("DNAM"):
                    mTransport.add(esm);
                    break;
                case AI_Wander:
                case AI_Activate:
                case AI_Escort:
                case AI_Follow:
                case AI_Travel:
                case AI_CNDT:
                    mAiPackage.add(esm);
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                case fourCC("INDX"):
                    // seems to occur only in .ESS files, unsure of purpose
                    int index;
                    esm.getHT(index);
                    Log(Debug::Warning) << "Creature::load: Unhandled INDX " << index;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasNpdt && !isDeleted)
            esm.fail("Missing NPDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }

    void Creature::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("CNAM", mOriginal);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNT("NPDT", mData, 96);
        esm.writeHNT("FLAG", ((mBloodType << 10) + mFlags));
        if (mScale != 1.0) {
            esm.writeHNT("XSCL", mScale);
        }

        mInventory.save(esm);
        mSpells.save(esm);
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
        mTransport.save(esm);
        mAiPackage.save(esm);
    }

    void Creature::blank()
    {
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mLevel = 0;
        mData.mStrength = mData.mIntelligence = mData.mWillpower = mData.mAgility =
            mData.mSpeed = mData.mEndurance = mData.mPersonality = mData.mLuck = 0;
        mData.mHealth = mData.mMana = mData.mFatigue = 0;
        mData.mSoul = 0;
        mData.mCombat = mData.mMagic = mData.mStealth = 0;
        for (int i=0; i<6; ++i) mData.mAttack[i] = 0;
        mData.mGold = 0;
        mBloodType = 0;
        mFlags = 0;
        mScale = 1.f;
        mModel.clear();
        mName.clear();
        mScript.clear();
        mOriginal.clear();
        mInventory.mList.clear();
        mSpells.mList.clear();
        mAiData.blank();
        mAiData.mFight = 90;
        mAiData.mFlee = 20;
        mAiPackage.mList.clear();
        mTransport.mList.clear();
    }

    const std::vector<Transport::Dest>& Creature::getTransport() const
    {
        return mTransport.mList;
    }
}
