#ifndef GAME_MWMECHANICS_NPCSTATS_H
#define GAME_MWMECHANICS_NPCSTATS_H

#include <map>

namespace MWMechanics
{
    /// \brief Additional stats for NPCs
    ///
    /// For non-NPC-specific stats, see the CreatureStats struct.

    struct NpcStats
    {
        // NPCs other than the player can only have one faction. But for the sake of consistency
        // we use the same data structure for the PC and the NPCs.
        std::map<std::string, int> mFactionRank;
    };
}

#endif
