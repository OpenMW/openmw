#ifndef GAME_MWWORLD_NPC_H
#define GAME_MWWORLD_NPC_H

#include "class.hpp"

namespace MWWorld
{
    class Npc : public Class
    {
        public:

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats

            static void registerSelf();
    };
}

#endif
