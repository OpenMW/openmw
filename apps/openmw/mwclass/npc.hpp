#ifndef GAME_MWCLASS_NPC_H
#define GAME_MWCLASS_NPC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Npc : public MWWorld::Class
    {
        public:

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            static void registerSelf();
    };
}

#endif
