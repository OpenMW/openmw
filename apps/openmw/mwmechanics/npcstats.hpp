#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "creaturestats.hpp"

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
            SkillValue mSkill[ESM::Skill::Length]; // SkillValue.mProgress used by the player only

            int mReputation;
            int mCrimeId;

            // ----- used by the player only, maybe should be moved at some point -------
            int mBounty;
            int mWerewolfKills;
            /// Used only for the player and for NPC's with ranks, modified by scripts; other NPCs have maximum one faction defined in their NPC record
            std::map<std::string, int> mFactionRank;
            std::set<std::string> mExpelled;
            std::map<std::string, int> mFactionReputation;
            int mLevelProgress; // 0-10
            std::vector<int> mSkillIncreases; // number of skill increases for each attribute (resets after leveling up)
            std::vector<int> mSpecIncreases; // number of skill increases for each specialization (accumulates throughout the entire game)
            std::set<std::string> mUsedIds;
            // ---------------------------------------------------------------------------

            /// Countdown to getting damage while underwater
            float mTimeToStartDrowning;

            bool mIsWerewolf;

        public:

            NpcStats();

            int getBaseDisposition() const;
            void setBaseDisposition(int disposition);

            int getReputation() const;
            void setReputation(int reputation);

            int getCrimeId() const;
            void setCrimeId(int id);

            const SkillValue& getSkill (int index) const;
            SkillValue& getSkill (int index);
            void setSkill(int index, const SkillValue& value);

            int getFactionRank(const std::string &faction) const;
            const std::map<std::string, int>& getFactionRanks() const;

            /// Increase the rank in this faction by 1, if such a rank exists.
            void raiseRank(const std::string& faction);
            /// Lower the rank in this faction by 1, if such a rank exists.
            void lowerRank(const std::string& faction);
            /// Join this faction, setting the initial rank to 0.
            void joinFaction(const std::string& faction);

            const std::set<std::string>& getExpelled() const { return mExpelled; }
            bool getExpelled(const std::string& factionID) const;
            void expell(const std::string& factionID);
            void clearExpelled(const std::string& factionID);

            bool isInFaction (const std::string& faction) const;

            float getSkillProgressRequirement (int skillIndex, const ESM::Class& class_) const;

            void useSkill (int skillIndex, const ESM::Class& class_, int usageType = -1, float extraFactor=1.f);
            ///< Increase skill by usage.

            void increaseSkill (int skillIndex, const ESM::Class& class_, bool preserveProgress, bool readBook = false);

            int getLevelProgress() const;

            int getLevelupAttributeMultiplier(int attribute) const;

            int getSkillIncreasesForSpecialization(int spec) const;

            void levelUp();

            void updateHealth();
            ///< Calculate health based on endurance and strength.
            ///  Called at character creation.

            void flagAsUsed (const std::string& id);
            ///< @note Id must be lower-case

            bool hasBeenUsed (const std::string& id) const;
            ///< @note Id must be lower-case

            int getBounty() const;

            void setBounty (int bounty);

            int getFactionReputation (const std::string& faction) const;

            void setFactionReputation (const std::string& faction, int value);

            bool hasSkillsForRank (const std::string& factionId, int rank) const;

            bool isWerewolf() const;

            void setWerewolf(bool set);

            int getWerewolfKills() const;

            /// Increments mWerewolfKills by 1.
            void addWerewolfKill();

            float getTimeToStartDrowning() const;
            /// Sets time left for the creature to drown if it stays underwater.
            /// @param time value from [0,20]
            void setTimeToStartDrowning(float time);

            void writeState (ESM::CreatureStats& state) const;
            void writeState (ESM::NpcStats& state) const;

            void readState (const ESM::CreatureStats& state);
            void readState (const ESM::NpcStats& state);
    };
}

#endif
