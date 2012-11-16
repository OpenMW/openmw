#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "stat.hpp"
#include "drawstate.hpp"

namespace ESM
{
    struct Class;
}

namespace MWMechanics
{
    /// \brief Additional stats for NPCs
    ///
    /// For non-NPC-specific stats, see the CreatureStats struct.
    ///
    /// \note For technical reasons the spell list and the currently selected spell is also handled by
    /// CreatureStats, even though they are actually NPC stats.

    class NpcStats
    {
        public:

            enum Flag
            {
                Flag_ForceRun = 1,
                Flag_ForceSneak = 2,
                Flag_Run = 4,
                Flag_Sneak = 8
            };

        private:

            /// NPCs other than the player can only have one faction. But for the sake of consistency
            /// we use the same data structure for the PC and the NPCs.
            /// \note the faction key must be in lowercase
            std::map<std::string, int> mFactionRank;

            DrawState_ mDrawState;
            int mDisposition;
            unsigned int mMovementFlags;
            Stat<float> mSkill[27];
            int mBounty;
            std::set<std::string> mExpelled;
            std::map<std::string, int> mFactionReputation;
            bool mVampire;
            int mReputation;
            bool mWerewolf;
            int mWerewolfKills;

            int mLevelProgress; // 0-10

            std::vector<int> mSkillIncreases; // number of skill increases for each attribute

            std::set<std::string> mUsedIds;

        public:

            NpcStats();

            DrawState_ getDrawState() const;

            void setDrawState (DrawState_ state);

            int getBaseDisposition() const;

            void setBaseDisposition(int disposition);

            int getReputation() const;

            void setReputation(int reputation);

            bool getMovementFlag (Flag flag) const;

            void setMovementFlag (Flag flag, bool state);

            const Stat<float>& getSkill (int index) const;

            Stat<float>& getSkill (int index);

            std::map<std::string, int>& getFactionRanks();

            std::set<std::string>& getExpelled();

            bool isSameFaction (const NpcStats& npcStats) const;
            ///< Do *this and \a npcStats share a faction?

            const std::map<std::string, int>& getFactionRanks() const;

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

            void flagAsUsed (const std::string& id);

            bool hasBeenUsed (const std::string& id) const;

            int getBounty() const;

            void setBounty (int bounty);

            int getFactionReputation (const std::string& faction) const;

            void setFactionReputation (const std::string& faction, int value);

            bool isVampire() const;

            void setVampire (bool set);

            bool hasSkillsForRank (const std::string& factionId, int rank) const;

            bool isWerewolf() const;

            void setWerewolf (bool set);

            int getWerewolfKills() const;
    };
}

#endif
