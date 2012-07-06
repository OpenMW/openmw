#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>
#include <set>
#include <string>

#include "stat.hpp"
#include "drawstate.hpp"

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
            DrawState mDrawState;

        public:

        /// NPCs other than the player can only have one faction. But for the sake of consistency
        /// we use the same data structure for the PC and the NPCs.
        /// \note the faction key must be in lowercase
        std::map<std::string, int> mFactionRank;

        Stat<float> mSkill[27];

        bool mForceRun;
        bool mForceSneak;
        bool mRun;
        bool mSneak;

            NpcStats();

            DrawState getDrawState() const;

            void setDrawState (DrawState state);
    };
}

#endif
