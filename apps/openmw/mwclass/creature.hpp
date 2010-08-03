#ifndef GAME_MWCLASS_CREATURE_H
#define GAME_MWCLASS_CREATURE_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Creature : public MWWorld::Class
    {
        public:

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            static void registerSelf();
    };
}

#endif
