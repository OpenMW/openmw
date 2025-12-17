#include "creaturestats.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm3/loadmgef.hpp>

#include <limits>

namespace ESM
{

    void CreatureStats::load(ESMReader& esm)
    {
        const bool intFallback = esm.getFormatVersion() <= MaxIntFallbackFormatVersion;
        for (auto& attribute : mAttributes)
            attribute.load(esm, intFallback);

        for (auto& dynamic : mDynamic)
            dynamic.load(esm);

        mGoldPool = 0;
        esm.getHNOT(mGoldPool, "GOLD");

        mTradeTime.mDay = 0;
        mTradeTime.mHour = 0;
        if (esm.peekNextSub("TIME"))
            mTradeTime.load(esm);

        int32_t flags = 0;
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
        mRecalcDynamicStats = false;
        if (esm.getFormatVersion() <= MaxUnoptimizedCharacterDataFormatVersion)
        {
            esm.getHNOT(mDead, "DEAD");
            esm.getHNOT(mDeathAnimationFinished, "DFNT");
            esm.getHNOT(mDied, "DIED");
            esm.getHNOT(mMurdered, "MURD");
            esm.getHNOT(mTalkedTo, "TALK");
            esm.getHNOT(mAlarmed, "ALRM");
            esm.getHNOT(mAttacked, "ATKD");
            esm.getHNOT(mKnockdown, "KNCK");
            esm.getHNOT(mKnockdownOneFrame, "KNC1");
            esm.getHNOT(mKnockdownOverOneFrame, "KNCO");
            esm.getHNOT(mHitRecovery, "HITR");
            esm.getHNOT(mBlock, "BLCK");
        }
        else
        {
            esm.getHNOT(flags, "AFLG");
            mDead = flags & Dead;
            mDeathAnimationFinished = flags & DeathAnimationFinished;
            mDied = flags & Died;
            mMurdered = flags & Murdered;
            mTalkedTo = flags & TalkedTo;
            mAlarmed = flags & Alarmed;
            mAttacked = flags & Attacked;
            mKnockdown = flags & Knockdown;
            mKnockdownOneFrame = flags & KnockdownOneFrame;
            mKnockdownOverOneFrame = flags & KnockdownOverOneFrame;
            mHitRecovery = flags & HitRecovery;
            mBlock = flags & Block;
            mRecalcDynamicStats = flags & RecalcDynamicStats;
        }

        mMovementFlags = 0;
        esm.getHNOT(mMovementFlags, "MOVE");

        mFallHeight = 0;
        esm.getHNOT(mFallHeight, "FALL");

        mLastHitObject = esm.getHNORefId("LHIT");

        mLastHitAttemptObject = esm.getHNORefId("LHAT");

        if (esm.getFormatVersion() <= MaxUnoptimizedCharacterDataFormatVersion)
            esm.getHNOT(mRecalcDynamicStats, "CALC");

        mDrawState = 0;
        esm.getHNOT(mDrawState, "DRAW");

        mLevel = 1;
        esm.getHNOT(mLevel, "LEVL");

        mActorId = -1;
        esm.getHNOT(mActorId, "ACID");

        mDeathAnimation = -1;
        esm.getHNOT(mDeathAnimation, "DANM");

        mTimeOfDeath.mDay = 0;
        mTimeOfDeath.mHour = 0;
        if (esm.peekNextSub("DTIM"))
            mTimeOfDeath.load(esm, "DTIM");

        mSpells.load(esm);
        mActiveSpells.load(esm);
        mAiSequence.load(esm);
        mMagicEffects.load(esm);

        if (esm.getFormatVersion() <= MaxClearModifiersFormatVersion)
        {
            while (esm.isNextSub("SUMM"))
            {
                int32_t magicEffect;
                esm.getHT(magicEffect);
                ESM::RefId source = esm.getHNORefId("SOUR");
                int32_t effectIndex = -1;
                esm.getHNOT(effectIndex, "EIND");
                int32_t actorId;
                esm.getHNT(actorId, "ACID");
                mSummonedCreatureMap[SummonKey(ESM::MagicEffect::indexToRefId(magicEffect), source, effectIndex)] = actorId;
                mSummonedCreatures.emplace(ESM::MagicEffect::indexToRefId(
                    magicEffect), RefNum{ .mIndex = static_cast<uint32_t>(actorId), .mContentFile = -1 });
            }
        }
        else
        {
            while (esm.isNextSub("SUMM"))
            {
                int32_t magicEffect;
                esm.getHT(magicEffect);
                RefNum actor;
                if (esm.getFormatVersion() <= MaxActorIdSaveGameFormatVersion)
                    esm.getHNT(actor.mIndex, "ACID");
                else
                    actor = esm.getFormId(true, "ACID");
                mSummonedCreatures.emplace(ESM::MagicEffect::indexToRefId(magicEffect), actor);
            }
        }

        while (esm.isNextSub("GRAV"))
        {
            int32_t actorId;
            esm.getHT(actorId);
            mSummonGraveyard.push_back(actorId);
        }

        mHasAiSettings = false;
        esm.getHNOT(mHasAiSettings, "AISE");

        if (mHasAiSettings)
        {
            for (auto& setting : mAiSettings)
                setting.load(esm);
        }

        while (esm.isNextSub("CORP"))
        {
            ESM::RefId id = esm.getRefId();

            CorprusStats stats;
            esm.getHNT(stats.mWorsenings, "WORS");
            stats.mNextWorsening.load(esm);

            mCorprusSpells[id] = stats;
        }
        if (esm.getFormatVersion() <= MaxOldSkillsAndAttributesFormatVersion)
            mMissingACDT = mGoldPool == std::numeric_limits<int>::min();
        else
        {
            mMissingACDT = false;
            esm.getHNOT(mMissingACDT, "NOAC");
        }
    }

    void CreatureStats::save(ESMWriter& esm) const
    {
        for (const auto& attribute : mAttributes)
            attribute.save(esm);

        for (const auto& dynamic : mDynamic)
            dynamic.save(esm);

        if (mGoldPool)
            esm.writeHNT("GOLD", mGoldPool);

        if (mTradeTime.mDay != 0 || mTradeTime.mHour != 0)
            esm.writeHNT("TIME", mTradeTime);

        int32_t flags = 0;
        if (mDead)
            flags |= Dead;
        if (mDeathAnimationFinished)
            flags |= DeathAnimationFinished;
        if (mDied)
            flags |= Died;
        if (mMurdered)
            flags |= Murdered;
        if (mTalkedTo)
            flags |= TalkedTo;
        if (mAlarmed)
            flags |= Alarmed;
        if (mAttacked)
            flags |= Attacked;
        if (mKnockdown)
            flags |= Knockdown;
        if (mKnockdownOneFrame)
            flags |= KnockdownOneFrame;
        if (mKnockdownOverOneFrame)
            flags |= KnockdownOverOneFrame;
        if (mHitRecovery)
            flags |= HitRecovery;
        if (mBlock)
            flags |= Block;
        if (mRecalcDynamicStats)
            flags |= RecalcDynamicStats;

        if (flags)
            esm.writeHNT("AFLG", flags);

        if (mMovementFlags)
            esm.writeHNT("MOVE", mMovementFlags);

        if (mFallHeight)
            esm.writeHNT("FALL", mFallHeight);

        if (!mLastHitObject.empty())
            esm.writeHNRefId("LHIT", mLastHitObject);

        if (!mLastHitAttemptObject.empty())
            esm.writeHNRefId("LHAT", mLastHitAttemptObject);

        if (mDrawState)
            esm.writeHNT("DRAW", mDrawState);

        if (mLevel != 1)
            esm.writeHNT("LEVL", mLevel);

        if (mDeathAnimation != -1)
            esm.writeHNT("DANM", mDeathAnimation);

        if (mTimeOfDeath.mHour != 0 || mTimeOfDeath.mDay != 0)
            esm.writeHNT("DTIM", mTimeOfDeath);

        mSpells.save(esm);
        mActiveSpells.save(esm);
        mAiSequence.save(esm);
        mMagicEffects.save(esm);

        for (const auto& [effectId, actor] : mSummonedCreatures)
        {
            esm.writeHNT("SUMM", ESM::MagicEffect::refIdToIndex(effectId));
            esm.writeFormId(actor, true, "ACID");
        }

        esm.writeHNT("AISE", mHasAiSettings);
        if (mHasAiSettings)
        {
            for (const auto& setting : mAiSettings)
                setting.save(esm);
        }
        if (mMissingACDT)
            esm.writeHNT("NOAC", mMissingACDT);
    }

    void CreatureStats::blank()
    {
        mTradeTime.mHour = 0;
        mTradeTime.mDay = 0;
        mGoldPool = 0;
        mActorId = -1;
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
        mCorprusSpells.clear();
        mMissingACDT = false;
    }

}
