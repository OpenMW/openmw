#include "player.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Player::load (ESMReader &esm)
{
    mObject.mRef.loadId(esm, true);
    mObject.load (esm);

    mCellId.load (esm);

    esm.getHNTSized<12>(mLastKnownExteriorPosition, "LKEP");

    if (esm.isNextSub ("MARK"))
    {
        mHasMark = true;
        esm.getHTSized<24>(mMarkedPosition);
        mMarkedCell.load (esm);
    }
    else
        mHasMark = false;

    // Automove, no longer used.
    if (esm.isNextSub("AMOV"))
        esm.skipHSub();

    mBirthsign = esm.getHNString ("SIGN");

    mCurrentCrimeId = -1;
    esm.getHNOT (mCurrentCrimeId, "CURD");
    mPaidCrimeId = -1;
    esm.getHNOT (mPaidCrimeId, "PAYD");

    bool checkPrevItems = true;
    while (checkPrevItems)
    {
        std::string boundItemId = esm.getHNOString("BOUN");
        std::string prevItemId = esm.getHNOString("PREV");

        if (!boundItemId.empty())
            mPreviousItems[boundItemId] = prevItemId;
        else
            checkPrevItems = false;
    }

    if(esm.getFormat() < 19)
    {
        bool intFallback = esm.getFormat() < 11;
        bool clearModified = esm.getFormat() < 17 && !mObject.mNpcStats.mIsWerewolf;
        if (esm.hasMoreSubs())
        {
            for (int i=0; i<Attribute::Length; ++i)
            {
                StatState<float> attribute;
                attribute.load(esm, intFallback);
                if (clearModified)
                    attribute.mMod = 0.f;
                mSaveAttributes[i] = attribute.mBase + attribute.mMod - attribute.mDamage;
                if (mObject.mNpcStats.mIsWerewolf)
                    mObject.mCreatureStats.mAttributes[i] = attribute;
            }
            for (int i=0; i<Skill::Length; ++i)
            {
                StatState<float> skill;
                skill.load(esm, intFallback);
                if (clearModified)
                    skill.mMod = 0.f;
                mSaveSkills[i] = skill.mBase + skill.mMod - skill.mDamage;
                if (mObject.mNpcStats.mIsWerewolf)
                {
                    if(i == Skill::Acrobatics)
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

void Player::save (ESMWriter &esm) const
{
    mObject.save (esm);

    mCellId.save (esm);

    esm.writeHNT ("LKEP", mLastKnownExteriorPosition);

    if (mHasMark)
    {
        esm.writeHNT ("MARK", mMarkedPosition, 24);
        mMarkedCell.save (esm);
    }

    esm.writeHNString ("SIGN", mBirthsign);

    esm.writeHNT ("CURD", mCurrentCrimeId);
    esm.writeHNT ("PAYD", mPaidCrimeId);

    for (PreviousItems::const_iterator it=mPreviousItems.begin(); it != mPreviousItems.end(); ++it)
    {
        esm.writeHNString ("BOUN", it->first);
        esm.writeHNString ("PREV", it->second);
    }

    esm.writeHNT("WWAT", mSaveAttributes);
    esm.writeHNT("WWSK", mSaveSkills);
}

}
