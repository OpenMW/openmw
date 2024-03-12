#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include "creaturestats.hpp"
#include <components/esm/refid.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadskil.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ESM
{
    struct Class;
    struct NpcStats;
}

namespace MWMechanics
{
    /// \brief Additional stats for NPCs

    class NpcStats : public CreatureStats
    {
        int mDisposition;
        int mCrimeDispositionModifier;
        std::map<ESM::RefId, SkillValue> mSkills; // SkillValue.mProgress used by the player only

        int mReputation;
        int mCrimeId;

        // ----- used by the player only, maybe should be moved at some point -------
        int mBounty;
        int mWerewolfKills;
        /// Used only for the player and for NPC's with ranks, modified by scripts; other NPCs have maximum one faction
        /// defined in their NPC record
        std::map<ESM::RefId, int> mFactionRank;
        std::set<ESM::RefId> mExpelled;
        std::map<ESM::RefId, int> mFactionReputation;
        int mLevelProgress; // 0-10
        std::map<ESM::RefId, int>
            mSkillIncreases; // number of skill increases for each attribute (resets after leveling up)
        std::vector<int> mSpecIncreases; // number of skill increases for each specialization (accumulates throughout
                                         // the entire game)
        std::set<ESM::RefId> mUsedIds;
        // ---------------------------------------------------------------------------

        /// Countdown to getting damage while underwater
        float mTimeToStartDrowning;

        bool mIsWerewolf;

    public:
        NpcStats();

        int getBaseDisposition() const;
        void setBaseDisposition(int disposition);

        int getCrimeDispositionModifier() const;
        void setCrimeDispositionModifier(int value);
        void modCrimeDispositionModifier(int value);

        int getReputation() const;
        void setReputation(int reputation);

        int getCrimeId() const;
        void setCrimeId(int id);

        const SkillValue& getSkill(ESM::RefId id) const;
        SkillValue& getSkill(ESM::RefId id);
        void setSkill(ESM::RefId id, const SkillValue& value);

        int getFactionRank(const ESM::RefId& faction) const;
        const std::map<ESM::RefId, int>& getFactionRanks() const;

        /// Join this faction, setting the initial rank to 0.
        void joinFaction(const ESM::RefId& faction);
        /// Sets the rank in this faction to a specified value, if such a rank exists.
        void setFactionRank(const ESM::RefId& faction, int value);

        const std::set<ESM::RefId>& getExpelled() const { return mExpelled; }
        bool getExpelled(const ESM::RefId& factionID) const;
        void expell(const ESM::RefId& factionID, bool printMessage);
        void clearExpelled(const ESM::RefId& factionID);

        bool isInFaction(const ESM::RefId& faction) const;

        float getSkillProgressRequirement(ESM::RefId id, const ESM::Class& class_) const;

        int getLevelProgress() const;
        void setLevelProgress(int progress);

        int getLevelupAttributeMultiplier(ESM::Attribute::AttributeID attribute) const;
        int getSkillIncreasesForAttribute(ESM::Attribute::AttributeID attribute) const;
        void setSkillIncreasesForAttribute(ESM::Attribute::AttributeID, int increases);

        int getSkillIncreasesForSpecialization(ESM::Class::Specialization spec) const;
        void setSkillIncreasesForSpecialization(ESM::Class::Specialization spec, int increases);

        void levelUp();

        void updateHealth();
        ///< Calculate health based on endurance and strength.
        ///  Called at character creation.

        void flagAsUsed(const ESM::RefId& id);
        ///< @note Id must be lower-case

        bool hasBeenUsed(const ESM::RefId& id) const;
        ///< @note Id must be lower-case

        int getBounty() const;

        void setBounty(int bounty);

        int getFactionReputation(const ESM::RefId& faction) const;

        void setFactionReputation(const ESM::RefId& faction, int value);

        bool hasSkillsForRank(const ESM::RefId& factionId, int rank) const;

        bool isWerewolf() const;

        void setWerewolf(bool set);

        int getWerewolfKills() const;

        /// Increments mWerewolfKills by 1.
        void addWerewolfKill();

        float getTimeToStartDrowning() const;
        /// Sets time left for the creature to drown if it stays underwater.
        /// @param time value from [0,20]
        void setTimeToStartDrowning(float time);

        void writeState(ESM::CreatureStats& state) const;
        void writeState(ESM::NpcStats& state) const;

        void readState(const ESM::CreatureStats& state);
        void readState(const ESM::NpcStats& state);

        const std::map<ESM::RefId, SkillValue>& getSkills() const { return mSkills; }
    };
}

#endif
