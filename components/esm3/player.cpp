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

        esm.getHNT("LKEP", mLastKnownExteriorPosition);

        mHasMark = esm.getOptionalComposite("MARK", mMarkedPosition);
        if (mHasMark)
            mMarkedCell = esm.getCellId();

        // Automove, no longer used.
        if (esm.isNextSub("AMOV"))
            esm.skipHSub();

        mBirthsign = esm.getHNRefId("SIGN");

        mCurrentCrimeId = -1;
        esm.getHNOT(mCurrentCrimeId, "CURD");
        mPaidCrimeId = -1;
        esm.getHNOT(mPaidCrimeId, "PAYD");

        while (esm.peekNextSub("BOUN"))
        {
            ESM::RefId boundItemId = esm.getHNRefId("BOUN");
            ESM::RefId prevItemId = esm.getHNRefId("PREV");

            mPreviousItems[boundItemId] = prevItemId;
        }

        if (esm.getFormatVersion() <= MaxOldSkillsAndAttributesFormatVersion)
        {
            const bool intFallback = esm.getFormatVersion() <= MaxIntFallbackFormatVersion;
            const bool clearModified
                = esm.getFormatVersion() <= MaxClearModifiersFormatVersion && !mObject.mNpcStats.mIsWerewolf;
            if (esm.hasMoreSubs())
            {
                for (size_t i = 0; i < std::size(mSaveAttributes); ++i)
                {
                    StatState<float> attribute;
                    attribute.load(esm, intFallback);
                    if (clearModified)
                        attribute.mMod = 0.f;
                    mSaveAttributes[i] = attribute.mBase + attribute.mMod - attribute.mDamage;
                    if (mObject.mNpcStats.mIsWerewolf)
                        mObject.mCreatureStats.mAttributes[i] = attribute;
                }
                for (size_t i = 0; i < std::size(mSaveSkills); ++i)
                {
                    StatState<float> skill;
                    skill.load(esm, intFallback);
                    if (clearModified)
                        skill.mMod = 0.f;
                    mSaveSkills[i] = skill.mBase + skill.mMod - skill.mDamage;
                    if (mObject.mNpcStats.mIsWerewolf)
                    {
                        constexpr int acrobatics = 20;
                        if (i == acrobatics)
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
            esm.writeNamedComposite("MARK", mMarkedPosition);
            esm.writeCellId(mMarkedCell);
        }

        esm.writeHNRefId("SIGN", mBirthsign);

        esm.writeHNT("CURD", mCurrentCrimeId);
        esm.writeHNT("PAYD", mPaidCrimeId);

        for (const auto& [bound, prev] : mPreviousItems)
        {
            esm.writeHNRefId("BOUN", bound);
            esm.writeHNRefId("PREV", prev);
        }

        esm.writeHNT("WWAT", mSaveAttributes);
        esm.writeHNT("WWSK", mSaveSkills);
    }

}
