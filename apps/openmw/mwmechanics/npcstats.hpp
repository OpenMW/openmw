#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>
#include <set>
#include <string>

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
            unsigned int mMovementFlags;
            Stat<float> mSkill[27];

        public:

            NpcStats();

            DrawState_ getDrawState() const;

            void setDrawState (DrawState_ state);

            bool getMovementFlag (Flag flag) const;

            void setMovementFlag (Flag flag, bool state);

            const Stat<float>& getSkill (int index) const;

            Stat<float>& getSkill (int index);

            std::map<std::string, int>& getFactionRanks();

            const std::map<std::string, int>& getFactionRanks() const;

            float getSkillGain (int skillIndex, const ESM::Class& class_, int usageType = -1,
                int level = -1) const;
            ///< \param usageType: Usage specific factor, specified in the respective skill record;
            /// -1: use a factor of 1.0 instead.
            /// \param level Level to base calculation on; -1: use current level.

            void useSkill (int skillIndex, const ESM::Class& class_, int usageType = -1);
            ///< Increase skill by usage.
    };
}

#endif
