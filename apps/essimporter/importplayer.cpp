#include "importplayer.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void PCDT::load(ESM::ESMReader& esm)
    {
        while (esm.isNextSub("DNAM"))
        {
            mKnownDialogueTopics.push_back(esm.getHString());
        }

        mHasMark = false;
        if (esm.isNextSub("MNAM"))
        {
            mHasMark = true;
            mMNAM = esm.getHString();
        }

        esm.getHNT("PNAM", mPNAM.mPlayerFlags, mPNAM.mLevelProgress, mPNAM.mSkillProgress, mPNAM.mSkillIncreases,
            mPNAM.mTelekinesisRangeBonus, mPNAM.mVisionBonus, mPNAM.mDetectKeyMagnitude,
            mPNAM.mDetectEnchantmentMagnitude, mPNAM.mDetectAnimalMagnitude, mPNAM.mMarkLocation.mX,
            mPNAM.mMarkLocation.mY, mPNAM.mMarkLocation.mZ, mPNAM.mMarkLocation.mRotZ, mPNAM.mMarkLocation.mCellX,
            mPNAM.mMarkLocation.mCellY, mPNAM.mUnknown3, mPNAM.mVerticalRotation.mData, mPNAM.mSpecIncreases,
            mPNAM.mUnknown4);

        if (esm.isNextSub("SNAM"))
            esm.skipHSub();
        if (esm.isNextSub("NAM9"))
            esm.skipHSub();

        // Rest state. You shouldn't even be able to save during rest, but skip just in case.
        if (esm.isNextSub("RNAM"))
            /*
                int hoursLeft;
                float x, y, z; // resting position
            */
            esm.skipHSub(); // 16 bytes

        mBounty = 0;
        esm.getHNOT(mBounty, "CNAM");

        mBirthsign = esm.getHNOString("BNAM");

        // Holds the names of the last used Alchemy apparatus. Don't need to import this ATM,
        // because our GUI auto-selects the best apparatus.
        if (esm.isNextSub("NAM0"))
            esm.skipHSub();
        if (esm.isNextSub("NAM1"))
            esm.skipHSub();
        if (esm.isNextSub("NAM2"))
            esm.skipHSub();
        if (esm.isNextSub("NAM3"))
            esm.skipHSub();

        mHasENAM = esm.getHNOT("ENAM", mENAM.mCellX, mENAM.mCellY);

        if (esm.isNextSub("LNAM"))
            esm.skipHSub();

        while (esm.isNextSub("FNAM"))
        {
            FNAM fnam;
            esm.getHT(
                fnam.mRank, fnam.mUnknown1, fnam.mReputation, fnam.mFlags, fnam.mUnknown2, fnam.mFactionName.mData);
            mFactions.push_back(fnam);
        }

        mHasAADT = esm.getHNOT("AADT", mAADT.animGroupIndex, mAADT.mUnknown5); // Attack animation data?

        if (esm.isNextSub("KNAM"))
            esm.skipHSub(); // assigned Quick Keys, I think

        if (esm.isNextSub("ANIS"))
            esm.skipHSub(); // 16 bytes

        if (esm.isNextSub("WERE"))
        {
            // some werewolf data, 152 bytes
            // maybe current skills and attributes for werewolf form
            esm.getSubHeader();
            esm.skip(152);
        }
    }

}
