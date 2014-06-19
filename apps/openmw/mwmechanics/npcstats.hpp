#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "stat.hpp"

#include "creaturestats.hpp"

namespace ESM
{
    struct Class;
    struct NpcStats;
}

namespace MWMechanics
{
    /// \brief Additional stats for NPCs
    ///
    /// \note For technical reasons the spell list and the currently selected spell is also handled by
    /// CreatureStats, even though they are actually NPC stats.

    class NpcStats : public CreatureStats
    {
            /// NPCs other than the player can only have one faction. But for the sake of consistency
            /// we use the same data structure for the PC and the NPCs.
            /// \note the faction key must be in lowercase
            std::map<std::string, int> mFactionRank;

            int mDisposition;
            SkillValue mSkill[ESM::Skill::Length];
            SkillValue mWerewolfSkill[ESM::Skill::Length];
            int mBounty;
            std::set<std::string> mExpelled;
            std::map<std::string, int> mFactionReputation;
            int mReputation;
            int mCrimeId;
            int mWerewolfKills;
            int mProfit;

            int mLevelProgress; // 0-10

            std::vector<int> mSkillIncreases; // number of skill increases for each attribute

            std::set<std::string> mUsedIds;

            /// Countdown to getting damage while underwater
            float mTimeToStartDrowning;
            /// time since last hit from drowning
            float mLastDrowningHit;

            float mLevelHealthBonus;

        public:

            NpcStats();

            /// for mercenary companions. starts out as 0, and changes when items are added or removed through the UI.
            int getProfit() const;
            void modifyProfit(int diff);

            int getBaseDisposition() const;
            void setBaseDisposition(int disposition);

            int getReputation() const;
            void setReputation(int reputation);

            int getCrimeId() const;
            void setCrimeId(int id);

            const SkillValue& getSkill (int index) const;
            SkillValue& getSkill (int index);

            const std::map<std::string, int>& getFactionRanks() const;
            std::map<std::string, int>& getFactionRanks();

            const std::set<std::string>& getExpelled() const { return mExpelled; }
            bool getExpelled(const std::string& factionID) const;
            void expell(const std::string& factionID);
            void clearExpelled(const std::string& factionID);

            bool isSameFaction (const NpcStats& npcStats) const;
            ///< Do *this and \a npcStats share a faction?

            float getSkillGain (int skillIndex, const ESM::Class& class_, int usageType = -1,
                int level = -1) const;
            ///< \param usageType: Usage specific factor, specified in the respective skill record;
            /// -1: use a factor of 1.0 instead.
            /// \param level Level to base calculation on; -1: use current level.

            void useSkill (int skillIndex, const ESM::Class& class_, int usageType = -1);
            ///< Increase skill by usage.

            void increaseSkill (int skillIndex, const ESM::Class& class_, bool preserveProgress);

            int getLevelProgress() const;

            int getLevelupAttributeMultiplier(int attribute) const;

            void levelUp();

            void updateHealth();
            ///< Calculate health based on endurance and strength.
            ///  Called at character creation and at level up.

            void flagAsUsed (const std::string& id);

            bool hasBeenUsed (const std::string& id) const;

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

            void writeState (ESM::NpcStats& state) const;

            void readState (const ESM::NpcStats& state);
    };
}

#endif
