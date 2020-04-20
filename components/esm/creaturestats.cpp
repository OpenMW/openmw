#include "creaturestats.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

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

    mDeathAnimationFinished = false;
    esm.getHNOT (mDeathAnimationFinished, "DFNT");

    if (esm.getFormat() < 3 && mDead)
        mDeathAnimationFinished = true;

    mDied = false;
    esm.getHNOT (mDied, "DIED");

    mMurdered = false;
    esm.getHNOT (mMurdered, "MURD");

    if (esm.isNextSub("FRHT"))
        esm.skipHSub(); // Friendly hits, no longer used

    mTalkedTo = false;
    esm.getHNOT (mTalkedTo, "TALK");

    mAlarmed = false;
    esm.getHNOT (mAlarmed, "ALRM");

    mAttacked = false;
    esm.getHNOT (mAttacked, "ATKD");

    if (esm.isNextSub("HOST"))
        esm.skipHSub(); // Hostile, no longer used

    if (esm.isNextSub("ATCK"))
        esm.skipHSub(); // attackingOrSpell, no longer used

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

    if (esm.isNextSub("ASTR"))
        esm.skipHSub(); // attackStrength, no longer used

    mFallHeight = 0;
    esm.getHNOT (mFallHeight, "FALL");

    mLastHitObject = esm.getHNOString ("LHIT");

    mLastHitAttemptObject = esm.getHNOString ("LHAT");

    mRecalcDynamicStats = false;
    esm.getHNOT (mRecalcDynamicStats, "CALC");

    mDrawState = 0;
    esm.getHNOT (mDrawState, "DRAW");

    mLevel = 1;
    esm.getHNOT (mLevel, "LEVL");

    mActorId = -1;
    esm.getHNOT (mActorId, "ACID");

    //mHitAttemptActorId = -1;
    //esm.getHNOT(mHitAttemptActorId, "HAID");

    mDeathAnimation = -1;
    esm.getHNOT (mDeathAnimation, "DANM");

    mTimeOfDeath.mDay = 0;
    mTimeOfDeath.mHour = 0;
    esm.getHNOT (mTimeOfDeath, "DTIM");

    mSpells.load(esm);
    mActiveSpells.load(esm);
    mAiSequence.load(esm);
    mMagicEffects.load(esm);

    while (esm.isNextSub("SUMM"))
    {
        int magicEffect;
        esm.getHT(magicEffect);
        std::string source = esm.getHNOString("SOUR");
        int actorId;
        esm.getHNT (actorId, "ACID");
        mSummonedCreatureMap[std::make_pair(magicEffect, source)] = actorId;
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

    if (mTradeTime.mDay != 0 || mTradeTime.mHour != 0)
        esm.writeHNT ("TIME", mTradeTime);

    if (mDead)
        esm.writeHNT ("DEAD", mDead);

    if (mDeathAnimationFinished)
        esm.writeHNT ("DFNT", mDeathAnimationFinished);

    if (mDied)
        esm.writeHNT ("DIED", mDied);

    if (mMurdered)
        esm.writeHNT ("MURD", mMurdered);

    if (mTalkedTo)
        esm.writeHNT ("TALK", mTalkedTo);

    if (mAlarmed)
        esm.writeHNT ("ALRM", mAlarmed);

    if (mAttacked)
        esm.writeHNT ("ATKD", mAttacked);

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

    if (mFallHeight)
        esm.writeHNT ("FALL", mFallHeight);

    if (!mLastHitObject.empty())
        esm.writeHNString ("LHIT", mLastHitObject);

    if (!mLastHitAttemptObject.empty())
        esm.writeHNString ("LHAT", mLastHitAttemptObject);

    if (mRecalcDynamicStats)
        esm.writeHNT ("CALC", mRecalcDynamicStats);

    if (mDrawState)
        esm.writeHNT ("DRAW", mDrawState);

    if (mLevel != 1)
        esm.writeHNT ("LEVL", mLevel);

    if (mActorId != -1)
        esm.writeHNT ("ACID", mActorId);

    //if (mHitAttemptActorId != -1)
    //    esm.writeHNT("HAID", mHitAttemptActorId);

    if (mDeathAnimation != -1)
        esm.writeHNT ("DANM", mDeathAnimation);

    if (mTimeOfDeath.mHour != 0 && mTimeOfDeath.mDay != 0)
        esm.writeHNT ("DTIM", mTimeOfDeath);

    mSpells.save(esm);
    mActiveSpells.save(esm);
    mAiSequence.save(esm);
    mMagicEffects.save(esm);

    for (std::map<std::pair<int, std::string>, int>::const_iterator it = mSummonedCreatureMap.begin(); it != mSummonedCreatureMap.end(); ++it)
    {
        esm.writeHNT ("SUMM", it->first.first);
        esm.writeHNString ("SOUR", it->first.second);
        esm.writeHNT ("ACID", it->second);
    }

    for (std::vector<int>::const_iterator it = mSummonGraveyard.begin(); it != mSummonGraveyard.end(); ++it)
    {
        esm.writeHNT ("GRAV", *it);
    }

    esm.writeHNT("AISE", mHasAiSettings);
    if (mHasAiSettings)
    {
        for (int i=0; i<4; ++i)
            mAiSettings[i].save(esm);
    }
}

void ESM::CreatureStats::blank()
{
    mTradeTime.mHour = 0;
    mTradeTime.mDay = 0;
    mGoldPool = 0;
    mActorId = -1;
    //mHitAttemptActorId = -1;
    mHasAiSettings = false;
    mDead = false;
    mDeathAnimationFinished = false;
    mDied = false;
    mMurdered = false;
    mTalkedTo = false;
    mAlarmed = false;
    mAttacked = false;
    mKnockdown = false;
    mKnockdownOneFrame = false;
    mKnockdownOverOneFrame = false;
    mHitRecovery = false;
    mBlock = false;
    mMovementFlags = 0;
    mFallHeight = 0.f;
    mRecalcDynamicStats = false;
    mDrawState = 0;
    mDeathAnimation = -1;
    mLevel = 1;
}
