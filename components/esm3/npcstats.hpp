#ifndef OPENMW_ESM_NPCSTATS_H
#define OPENMW_ESM_NPCSTATS_H

#include "statstate.hpp"
#include <components/esm/refid.hpp>
#include <map>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct NpcStats
    {
        struct Faction
        {
            bool mExpelled;
            int mRank;
            int mReputation;

            Faction();
        };

        bool mIsWerewolf;

        bool mWerewolfDeprecatedData;

        std::map<ESM::RefId, Faction> mFactions; // lower case IDs
        int mDisposition;
        StatState<float> mSkills[27];
        int mBounty;
        int mReputation;
        int mWerewolfKills;
        int mLevelProgress;
        int mSkillIncrease[8];
        int mSpecIncreases[3];
        std::vector<ESM::RefId> mUsedIds; // lower case IDs
        float mTimeToStartDrowning;
        int mCrimeId;

        /// Initialize to default state
        void blank();

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
