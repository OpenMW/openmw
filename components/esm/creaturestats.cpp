#include "creaturestats.hpp"

void ESM::CreatureStats::load (ESMReader &esm)
{
    for (int i=0; i<8; ++i)
        mAttributes[i].load (esm);

    for (int i=0; i<3; ++i)
        mDynamic[i].load (esm);

    mGoldPool = 0;
    esm.getHNOT (mGoldPool, "GOLD");

    mTradeTime.mDay = 0;
    mTradeTime.mHour = 0;
    esm.getHNOT (mTradeTime, "TIME");

    mDead = false;
    esm.getHNOT (mDead, "DEAD");

    mDied = false;
    esm.getHNOT (mDied, "DIED");

    mMurdered = false;
    esm.getHNOT (mMurdered, "MURD");

    mFriendlyHits = 0;
    esm.getHNOT (mFriendlyHits, "FRHT");

    mTalkedTo = false;
    esm.getHNOT (mTalkedTo, "TALK");

    mAlarmed = false;
    esm.getHNOT (mAlarmed, "ALRM");

    mAttacked = false;
    esm.getHNOT (mAttacked, "ATKD");

    if (esm.isNextSub("HOST"))
        esm.skipHSub(); // Hostile, no longer used

    mAttackingOrSpell = false;
    esm.getHNOT (mAttackingOrSpell, "ATCK");

    mKnockdown = false;
    esm.getHNOT (mKnockdown, "KNCK");

    mKnockdownOneFrame = false;
    esm.getHNOT (mKnockdownOneFrame, "KNC1");

    mKnockdownOverOneFrame = false;
    esm.getHNOT (mKnockdownOverOneFrame, "KNCO");

    mHitRecovery = false;
    esm.getHNOT (mHitRecovery, "HITR");

    mBlock = false;
    esm.getHNOT (mBlock, "BLCK");

    mMovementFlags = 0;
    esm.getHNOT (mMovementFlags, "MOVE");

    mAttackStrength = 0;
    esm.getHNOT (mAttackStrength, "ASTR");

    mFallHeight = 0;
    esm.getHNOT (mFallHeight, "FALL");

    mLastHitObject = esm.getHNOString ("LHIT");

    mRecalcDynamicStats = false;
    esm.getHNOT (mRecalcDynamicStats, "CALC");

    mDrawState = 0;
    esm.getHNOT (mDrawState, "DRAW");

    mLevel = 1;
    esm.getHNOT (mLevel, "LEVL");

    mActorId = -1;
    esm.getHNOT (mActorId, "ACID");

    mDeathAnimation = 0;
    esm.getHNOT (mDeathAnimation, "DANM");

    mSpells.load(esm);
    mActiveSpells.load(esm);
    mAiSequence.load(esm);

    while (esm.isNextSub("SUMM"))
    {
        int magicEffect;
        esm.getHT(magicEffect);
        int actorId;
        esm.getHNT (actorId, "ACID");
        mSummonedCreatureMap[magicEffect] = actorId;
    }

    while (esm.isNextSub("GRAV"))
    {
        int actorId;
        esm.getHT(actorId);
        mSummonGraveyard.push_back(actorId);
    }

    mHasAiSettings = false;
    esm.getHNOT(mHasAiSettings, "AISE");

    if (mHasAiSettings)
    {
        for (int i=0; i<4; ++i)
            mAiSettings[i].load(esm);
    }
}

void ESM::CreatureStats::save (ESMWriter &esm) const
{

    for (int i=0; i<8; ++i)
        mAttributes[i].save (esm);

    for (int i=0; i<3; ++i)
        mDynamic[i].save (esm);

    if (mGoldPool)
        esm.writeHNT ("GOLD", mGoldPool);

    esm.writeHNT ("TIME", mTradeTime);

    if (mDead)
        esm.writeHNT ("DEAD", mDead);

    if (mDied)
        esm.writeHNT ("DIED", mDied);

    if (mMurdered)
        esm.writeHNT ("MURD", mMurdered);

    if (mFriendlyHits)
        esm.writeHNT ("FRHT", mFriendlyHits);

    if (mTalkedTo)
        esm.writeHNT ("TALK", mTalkedTo);

    if (mAlarmed)
        esm.writeHNT ("ALRM", mAlarmed);

    if (mAttacked)
        esm.writeHNT ("ATKD", mAttacked);

    if (mAttackingOrSpell)
        esm.writeHNT ("ATCK", mAttackingOrSpell);

    if (mKnockdown)
        esm.writeHNT ("KNCK", mKnockdown);

    if (mKnockdownOneFrame)
        esm.writeHNT ("KNC1", mKnockdownOneFrame);

    if (mKnockdownOverOneFrame)
        esm.writeHNT ("KNCO", mKnockdownOverOneFrame);

    if (mHitRecovery)
        esm.writeHNT ("HITR", mHitRecovery);

    if (mBlock)
        esm.writeHNT ("BLCK", mBlock);

    if (mMovementFlags)
        esm.writeHNT ("MOVE", mMovementFlags);

    if (mAttackStrength)
        esm.writeHNT ("ASTR", mAttackStrength);

    if (mFallHeight)
        esm.writeHNT ("FALL", mFallHeight);

    if (!mLastHitObject.empty())
        esm.writeHNString ("LHIT", mLastHitObject);

    if (mRecalcDynamicStats)
        esm.writeHNT ("CALC", mRecalcDynamicStats);

    if (mDrawState)
        esm.writeHNT ("DRAW", mDrawState);

    if (mLevel != 1)
        esm.writeHNT ("LEVL", mLevel);

    if (mActorId != -1)
        esm.writeHNT ("ACID", mActorId);

    if (mDeathAnimation)
        esm.writeHNT ("DANM", mDeathAnimation);

    mSpells.save(esm);
    mActiveSpells.save(esm);
    mAiSequence.save(esm);

    for (std::map<int, int>::const_iterator it = mSummonedCreatureMap.begin(); it != mSummonedCreatureMap.end(); ++it)
    {
        esm.writeHNT ("SUMM", it->first);
        esm.writeHNT ("ACID", it->second);
    }

    for (std::vector<int>::const_iterator it = mSummonGraveyard.begin(); it != mSummonGraveyard.end(); ++it)
    {
        esm.writeHNT ("GRAV", *it);
    }

    esm.writeHNT("AISE", mHasAiSettings);
    for (int i=0; i<4; ++i)
        mAiSettings[i].save(esm);
}
