#ifndef OPENMW_ESM_NPCSTATS_H
#define OPENMW_ESM_NPCSTATS_H

#include <string>
#include <vector>
#include <map>

#include "statstate.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct NpcStats
    {
        struct Skill
        {
            StatState<int> mRegular;
            StatState<int> mWerewolf;
        };

        struct Faction
        {
            bool mExpelled;
            int mRank;
            int mReputation;

            Faction();
        };

        std::map<std::string, Faction> mFactions;
        int mDisposition;
        Skill mSkills[27];
        int mBounty;
        int mReputation;
        int mWerewolfKills;
        int mProfit;
        float mAttackStrength;
        int mLevelProgress;
        int mSkillIncrease[8];
        std::vector<std::string> mUsedIds;
        float mTimeToStartDrowning;
        float mLastDrowningHit;
        float mLevelHealthBonus;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif