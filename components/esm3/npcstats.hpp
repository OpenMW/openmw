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
            int32_t mRank;
            int32_t mReputation;

            Faction();
        };

        bool mIsWerewolf;

        std::map<ESM::RefId, Faction> mFactions;
        int32_t mDisposition;
        int32_t mCrimeDispositionModifier;
        std::array<StatState<float>, ESM::Skill::Length> mSkills;
        int32_t mBounty;
        int32_t mReputation;
        int32_t mWerewolfKills;
        int32_t mLevelProgress;
        std::array<int32_t, ESM::Attribute::Length> mSkillIncrease;
        std::array<int32_t, 3> mSpecIncreases;
        std::vector<ESM::RefId> mUsedIds;
        float mTimeToStartDrowning;
        int32_t mCrimeId;

        /// Initialize to default state
        void blank();

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
