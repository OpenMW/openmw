#ifndef GAME_MWMECHANICS_ALCHEMY_H
#define GAME_MWMECHANICS_ALCHEMY_H

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{
    /// \brief Potion creatin via alchemy skill
    class Alchemy
    {
            MWWorld::Ptr mNpc;

        public:
        
            void setAlchemist (const MWWorld::Ptr& npc);
            ///< Set alchemist and configure alchemy setup accordingly. \a npc may be empty to indicate that
            /// there is no alchemist (alchemy session has ended).
    };
}

#endif

