#ifndef GAME_MWWORLD_CREATURE_H
#define GAME_MWWORLD_CREATURE_H

#include "class.hpp"

namespace MWWorld
{
    class Creature : public Class
    {
        public:

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats

            static void registerSelf();
    };
}

#endif
