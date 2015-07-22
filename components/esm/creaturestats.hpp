#ifndef OPENMW_ESM_CREATURESTATS_H
#define OPENMW_ESM_CREATURESTATS_H

#include <string>
#include <vector>
#include <map>

#include "statstate.hpp"

#include "defs.hpp"

#include "spellstate.hpp"
#include "activespells.hpp"
#include "magiceffects.hpp"
#include "aisequence.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct CreatureStats
    {
        StatState<int> mAttributes[8];
        StatState<float> mDynamic[3];

        MagicEffects mMagicEffects;

        AiSequence::AiSequence mAiSequence;

        bool mHasAiSettings;
        StatState<int> mAiSettings[4];

        std::map<std::pair<int, std::string>, int> mSummonedCreatureMap;
        std::vector<int> mSummonGraveyard;

        ESM::TimeStamp mTradeTime;
        int mGoldPool;
        int mActorId;

        bool mDead;
        bool mDied;
        bool mMurdered;
        bool mTalkedTo;
        bool mAlarmed;
        bool mAttacked;
        bool mKnockdown;
        bool mKnockdownOneFrame;
        bool mKnockdownOverOneFrame;
        bool mHitRecovery;
        bool mBlock;
        unsigned int mMovementFlags;
        float mFallHeight;
        std::string mLastHitObject;
        std::string mLastHitAttemptObject;
        bool mRecalcDynamicStats;
        int mDrawState;
        unsigned char mDeathAnimation;

        int mLevel;

        SpellState mSpells;
        ActiveSpells mActiveSpells;

        /// Initialize to default state
        void blank();

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
