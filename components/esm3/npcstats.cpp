#include "npcstats.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

    NpcStats::Faction::Faction()
        : mExpelled(false)
        , mRank(-1)
        , mReputation(0)
    {
    }

    void NpcStats::load(ESMReader& esm)
    {
        while (esm.isNextSub("FACT"))
        {
            ESM::RefId id = esm.getRefId();

            Faction faction;

            int expelled = 0;
            esm.getHNOT(expelled, "FAEX");

            if (expelled)
                faction.mExpelled = true;

            esm.getHNOT(faction.mRank, "FARA");

            esm.getHNOT(faction.mReputation, "FARE");

            mFactions.emplace(id, faction);
        }

        mDisposition = 0;
        esm.getHNOT(mDisposition, "DISP");

        const bool intFallback = esm.getFormatVersion() <= MaxIntFallbackFormatVersion;
        for (auto& skill : mSkills)
            skill.load(esm, intFallback);

        mWerewolfDeprecatedData = false;
        if (esm.getFormatVersion() <= MaxWerewolfDeprecatedDataFormatVersion && esm.peekNextSub("STBA"))
        {
            // we have deprecated werewolf skills, stored interleaved
            // Load into one big vector, then remove every 2nd value
            mWerewolfDeprecatedData = true;
            std::vector<StatState<float>> skills(mSkills.begin(), mSkills.end());

            for (size_t i = 0; i < std::size(mSkills); ++i)
            {
                StatState<float> skill;
                skill.load(esm, intFallback);
                skills.push_back(skill);
            }

            int i = 0;
            for (std::vector<StatState<float>>::iterator it = skills.begin(); it != skills.end(); ++i)
            {
                if (i % 2 == 1)
                    it = skills.erase(it);
                else
                    ++it;
            }
            if (skills.size() != std::size(mSkills))
                throw std::runtime_error(
                    "Invalid number of skill for werewolf deprecated data: " + std::to_string(skills.size()));
            std::copy(skills.begin(), skills.end(), mSkills.begin());
        }

        // No longer used
        bool hasWerewolfAttributes = false;
        esm.getHNOT(hasWerewolfAttributes, "HWAT");
        if (hasWerewolfAttributes)
        {
            StatState<int> dummy;
            for (int i = 0; i < ESM::Attribute::Length; ++i)
                dummy.load(esm, intFallback);
            mWerewolfDeprecatedData = true;
        }

        mIsWerewolf = false;
        esm.getHNOT(mIsWerewolf, "WOLF");

        mBounty = 0;
        esm.getHNOT(mBounty, "BOUN");

        mReputation = 0;
        esm.getHNOT(mReputation, "REPU");

        mWerewolfKills = 0;
        esm.getHNOT(mWerewolfKills, "WKIL");

        // No longer used
        if (esm.isNextSub("PROF"))
            esm.skipHSub(); // int profit

        // No longer used
        if (esm.isNextSub("ASTR"))
            esm.skipHSub(); // attackStrength

        mLevelProgress = 0;
        esm.getHNOT(mLevelProgress, "LPRO");

        mSkillIncrease.fill(0);
        esm.getHNOT(mSkillIncrease, "INCR");

        mSpecIncreases.fill(0);
        esm.getHNOT(mSpecIncreases, "SPEC");

        while (esm.isNextSub("USED"))
            mUsedIds.push_back(esm.getRefId());

        mTimeToStartDrowning = 0;
        esm.getHNOT(mTimeToStartDrowning, "DRTI");

        // No longer used
        float lastDrowningHit = 0;
        esm.getHNOT(lastDrowningHit, "DRLH");

        // No longer used
        float levelHealthBonus = 0;
        esm.getHNOT(levelHealthBonus, "LVLH");

        mCrimeId = -1;
        esm.getHNOT(mCrimeId, "CRID");
    }

    void NpcStats::save(ESMWriter& esm) const
    {
        for (auto iter(mFactions.begin()); iter != mFactions.end(); ++iter)
        {
            esm.writeHNRefId("FACT", iter->first);

            if (iter->second.mExpelled)
            {
                int expelled = 1;
                esm.writeHNT("FAEX", expelled);
            }

            if (iter->second.mRank >= 0)
                esm.writeHNT("FARA", iter->second.mRank);

            if (iter->second.mReputation)
                esm.writeHNT("FARE", iter->second.mReputation);
        }

        if (mDisposition)
            esm.writeHNT("DISP", mDisposition);

        for (const auto& skill : mSkills)
            skill.save(esm);

        if (mIsWerewolf)
            esm.writeHNT("WOLF", mIsWerewolf);

        if (mBounty)
            esm.writeHNT("BOUN", mBounty);

        if (mReputation)
            esm.writeHNT("REPU", mReputation);

        if (mWerewolfKills)
            esm.writeHNT("WKIL", mWerewolfKills);

        if (mLevelProgress)
            esm.writeHNT("LPRO", mLevelProgress);

        bool saveSkillIncreases = false;
        for (int increase : mSkillIncrease)
        {
            if (increase != 0)
            {
                saveSkillIncreases = true;
                break;
            }
        }
        if (saveSkillIncreases)
            esm.writeHNT("INCR", mSkillIncrease);

        if (mSpecIncreases[0] != 0 || mSpecIncreases[1] != 0 || mSpecIncreases[2] != 0)
            esm.writeHNT("SPEC", mSpecIncreases);

        for (auto iter(mUsedIds.begin()); iter != mUsedIds.end(); ++iter)
            esm.writeHNRefId("USED", *iter);

        if (mTimeToStartDrowning)
            esm.writeHNT("DRTI", mTimeToStartDrowning);

        if (mCrimeId != -1)
            esm.writeHNT("CRID", mCrimeId);
    }

    void NpcStats::blank()
    {
        mWerewolfDeprecatedData = false;
        mIsWerewolf = false;
        mDisposition = 0;
        mBounty = 0;
        mReputation = 0;
        mWerewolfKills = 0;
        mLevelProgress = 0;
        mSkillIncrease.fill(0);
        mSpecIncreases.fill(0);
        mTimeToStartDrowning = 20;
        mCrimeId = -1;
    }

}
