#ifndef OPENMW_ESM_CREATURESTATS_H
#define OPENMW_ESM_CREATURESTATS_H

#include <string>
#include <vector>
#include <map>

#include "statstate.hpp"

#include "defs.hpp"

#include "spellstate.hpp"
#include "activespells.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct CreatureStats
    {
        StatState<int> mAttributes[8];
        StatState<float> mDynamic[3];

        ESM::TimeStamp mTradeTime;
        int mGoldPool;
        int mActorId;

        bool mDead;
        bool mDied;
        int mFriendlyHits;
        bool mTalkedTo;
        bool mAlarmed;
        bool mAttacked;
        bool mHostile;
        bool mAttackingOrSpell;
        bool mKnockdown;
        bool mKnockdownOneFrame;
        bool mKnockdownOverOneFrame;
        bool mHitRecovery;
        bool mBlock;
        unsigned int mMovementFlags;
        float mAttackStrength;
        float mFallHeight;
        std::string mLastHitObject;
        bool mRecalcDynamicStats;
        int mDrawState;

        int mLevel;

        SpellState mSpells;
        ActiveSpells mActiveSpells;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
