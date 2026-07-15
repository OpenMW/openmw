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

            int32_t expelled = 0;
            esm.getHNOT(expelled, "FAEX");

            if (expelled)
                faction.mExpelled = true;

            esm.getHNOT(faction.mRank, "FARA");

            esm.getHNOT(faction.mReputation, "FARE");

            mFactions.emplace(id, faction);
        }

        mDisposition = 0;
        esm.getHNOT(mDisposition, "DISP");

        mCrimeDispositionModifier = 0;
        esm.getHNOT(mCrimeDispositionModifier, "DISM");

        const bool intFallback = esm.getFormatVersion() <= MaxIntFallbackFormatVersion;
        for (int i = 0; i < ESM::Skill::Length; ++i)
            mSkills[ESM::Skill::indexToRefId(i)].load(esm, intFallback);

        mIsWerewolf = false;
        esm.getHNOT(mIsWerewolf, "WOLF");

        mBounty = 0;
        esm.getHNOT(mBounty, "BOUN");

        mReputation = 0;
        esm.getHNOT(mReputation, "REPU");

        mWerewolfKills = 0;
        esm.getHNOT(mWerewolfKills, "WKIL");

        mLevelProgress = 0;
        esm.getHNOT(mLevelProgress, "LPRO");

        mSkillIncrease.clear();
        if (esm.isNextSub("INCR"))
        {
            esm.getSubHeader();
            for (int i = 0; i < ESM::Attribute::Length; ++i)
                esm.getT(mSkillIncrease[ESM::Attribute::indexToRefId(i)]);
        }

        mSpecIncreases.fill(0);
        esm.getHNOT(mSpecIncreases, "SPEC");

        while (esm.isNextSub("USED"))
            mUsedIds.push_back(esm.getRefId());

        mTimeToStartDrowning = 0;
        esm.getHNOT(mTimeToStartDrowning, "DRTI");

        mCrimeId = -1;
        esm.getHNOT(mCrimeId, "CRID");
    }

    void NpcStats::save(ESMWriter& esm) const
    {
        for (const auto& [id, faction] : mFactions)
        {
            esm.writeHNRefId("FACT", id);

            if (faction.mExpelled)
            {
                int32_t expelled = 1;
                esm.writeHNT("FAEX", expelled);
            }

            if (faction.mRank >= 0)
                esm.writeHNT("FARA", faction.mRank);

            if (faction.mReputation)
                esm.writeHNT("FARE", faction.mReputation);
        }

        if (mDisposition)
            esm.writeHNT("DISP", mDisposition);

        if (mCrimeDispositionModifier)
            esm.writeHNT("DISM", mCrimeDispositionModifier);

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            const auto it = mSkills.find(ESM::Skill::indexToRefId(i));
            if (it != mSkills.end())
                it->second.save(esm);
            else
                StatState<float>{}.save(esm);
        }

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
        for (const auto& [id, increase] : mSkillIncrease)
        {
            if (increase != 0)
            {
                saveSkillIncreases = true;
                break;
            }
        }
        if (saveSkillIncreases)
        {
            esm.startSubRecord("INCR");
            for (int i = 0; i < ESM::Attribute::Length; ++i)
            {
                int32_t increase = 0;
                const auto it = mSkillIncrease.find(ESM::Attribute::indexToRefId(i));
                if (it != mSkillIncrease.end())
                    increase = it->second;
                esm.writeT(increase);
            }
            esm.endRecord("INCR");
        }

        if (mSpecIncreases[0] != 0 || mSpecIncreases[1] != 0 || mSpecIncreases[2] != 0)
            esm.writeHNT("SPEC", mSpecIncreases);

        for (const RefId& id : mUsedIds)
            esm.writeHNRefId("USED", id);

        if (mTimeToStartDrowning)
            esm.writeHNT("DRTI", mTimeToStartDrowning);

        if (mCrimeId != -1)
            esm.writeHNT("CRID", mCrimeId);
    }

    void NpcStats::blank()
    {
        mIsWerewolf = false;
        mDisposition = 0;
        mCrimeDispositionModifier = 0;
        mBounty = 0;
        mReputation = 0;
        mWerewolfKills = 0;
        mLevelProgress = 0;
        mSkillIncrease.clear();
        mSpecIncreases.fill(0);
        mTimeToStartDrowning = 20;
        mCrimeId = -1;
    }

}
