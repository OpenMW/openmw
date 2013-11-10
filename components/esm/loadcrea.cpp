#include "loadcrea.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM {

    unsigned int Creature::sRecordId = REC_CREA;

void Creature::load(ESMReader &esm)
{
    mPersistent = esm.getRecordFlags() & 0x0400;

    mModel = esm.getHNString("MODL");
    mOriginal = esm.getHNOString("CNAM");
    mName = esm.getHNOString("FNAM");
    mScript = esm.getHNOString("SCRI");

    esm.getHNT(mData, "NPDT", 96);

    esm.getHNT(mFlags, "FLAG");
    mScale = 1.0;
    esm.getHNOT(mScale, "XSCL");

    mInventory.load(esm);
    mSpells.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&mAiData, sizeof(mAiData));
        mHasAI = true;
    }
    else
        mHasAI = false;

    mAiPackage.load(esm);
    esm.skipRecord();
}

void Creature::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("CNAM", mOriginal);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNT("NPDT", mData, 96);
    esm.writeHNT("FLAG", mFlags);
    if (mScale != 1.0) {
        esm.writeHNT("XSCL", mScale);
    }

    mInventory.save(esm);
    mSpells.save(esm);
    if (mHasAI) {
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
    }
    mAiPackage.save(esm);
}

    void Creature::blank()
    {
        mData.mType = 0;
        mData.mLevel = 0;
        mData.mStrength = mData.mIntelligence = mData.mWillpower = mData.mAgility =
            mData.mSpeed = mData.mEndurance = mData.mPersonality = mData.mLuck = 0;
        mData.mHealth = mData.mMana = mData.mFatigue = 0;
        mData.mSoul = 0;
        mData.mCombat = mData.mMagic = mData.mStealth = 0;
        for (int i=0; i<6; ++i) mData.mAttack[i] = 0;
        mData.mGold = 0;
        mFlags = 0;
        mScale = 0;
        mModel.clear();
        mName.clear();
        mScript.clear();
        mOriginal.clear();
        mInventory.mList.clear();
        mSpells.mList.clear();
        mHasAI = false;
        mAiData.blank();
        mAiData.mServices = 0;
        mAiPackage.mList.clear();
    }
}
