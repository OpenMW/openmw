#include "player.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    void Player::load(ESMReader& esm)
    {
        mObject.mRef.loadId(esm, true);
        mObject.load(esm);

        mCellId = esm.getCellId();

        esm.getHNTSized<12>(mLastKnownExteriorPosition, "LKEP");

        if (esm.isNextSub("MARK"))
        {
            mHasMark = true;
            esm.getHTSized<24>(mMarkedPosition);
            mMarkedCell = esm.getCellId();
        }
        else
            mHasMark = false;

        // Automove, no longer used.
        if (esm.isNextSub("AMOV"))
            esm.skipHSub();

        mBirthsign = esm.getHNRefId("SIGN");

        mCurrentCrimeId = -1;
        esm.getHNOT(mCurrentCrimeId, "CURD");
        mPaidCrimeId = -1;
        esm.getHNOT(mPaidCrimeId, "PAYD");

        bool checkPrevItems = true;
        while (checkPrevItems)
        {
            ESM::RefId boundItemId = esm.getHNORefId("BOUN");
            ESM::RefId prevItemId = esm.getHNORefId("PREV");

            if (!boundItemId.empty())
                mPreviousItems[boundItemId] = prevItemId;
            else
                checkPrevItems = false;
        }

        if (esm.getFormatVersion() <= MaxOldSkillsAndAttributesFormatVersion)
        {
            const bool intFallback = esm.getFormatVersion() <= MaxIntFallbackFormatVersion;
            const bool clearModified
                = esm.getFormatVersion() <= MaxClearModifiersFormatVersion && !mObject.mNpcStats.mIsWerewolf;
            if (esm.hasMoreSubs())
            {
                for (int i = 0; i < Attribute::Length; ++i)
                {
                    StatState<float> attribute;
                    attribute.load(esm, intFallback);
                    if (clearModified)
                        attribute.mMod = 0.f;
                    mSaveAttributes[i] = attribute.mBase + attribute.mMod - attribute.mDamage;
                    if (mObject.mNpcStats.mIsWerewolf)
                        mObject.mCreatureStats.mAttributes[i] = attribute;
                }
                for (int i = 0; i < Skill::Length; ++i)
                {
                    StatState<float> skill;
                    skill.load(esm, intFallback);
                    if (clearModified)
                        skill.mMod = 0.f;
                    mSaveSkills[i] = skill.mBase + skill.mMod - skill.mDamage;
                    if (mObject.mNpcStats.mIsWerewolf)
                    {
                        if (i == Skill::Acrobatics)
                            mSetWerewolfAcrobatics = mObject.mNpcStats.mSkills[i].mBase != skill.mBase;
                        mObject.mNpcStats.mSkills[i] = skill;
                    }
                }
            }
        }
        else
        {
            mSetWerewolfAcrobatics = false;
            esm.getHNT(mSaveAttributes, "WWAT");
            esm.getHNT(mSaveSkills, "WWSK");
        }
    }

    void Player::save(ESMWriter& esm) const
    {
        mObject.save(esm);

        esm.writeCellId(mCellId);

        esm.writeHNT("LKEP", mLastKnownExteriorPosition);

        if (mHasMark)
        {
            esm.writeHNT("MARK", mMarkedPosition, 24);
            esm.writeCellId(mMarkedCell);
        }

        esm.writeHNRefId("SIGN", mBirthsign);

        esm.writeHNT("CURD", mCurrentCrimeId);
        esm.writeHNT("PAYD", mPaidCrimeId);

        for (PreviousItems::const_iterator it = mPreviousItems.begin(); it != mPreviousItems.end(); ++it)
        {
            esm.writeHNRefId("BOUN", it->first);
            esm.writeHNRefId("PREV", it->second);
        }

        esm.writeHNT("WWAT", mSaveAttributes);
        esm.writeHNT("WWSK", mSaveSkills);
    }

}
