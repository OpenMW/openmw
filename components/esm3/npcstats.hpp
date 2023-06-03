#ifndef OPENMW_ESM_NPCSTATS_H
#define OPENMW_ESM_NPCSTATS_H

#include "loadskil.hpp"
#include "statstate.hpp"
#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>

#include <array>
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
        std::array<StatState<float>, ESM::Skill::Length> mSkills;
        int mBounty;
        int mReputation;
        int mWerewolfKills;
        int mLevelProgress;
        std::array<int, ESM::Attribute::Length> mSkillIncrease;
        std::array<int, 3> mSpecIncreases;
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
