#ifndef GAME_MWMECHANICS_ACTORS_H
#define GAME_MWMECHANICS_ACTORS_H

#include <set>
#include <vector>
#include <string>

#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class Vector3;
}

namespace MWWorld
{
    class Environment;
}

namespace MWMechanics
{
    class Actors
    {
           MWWorld::Environment& mEnvironment;
           std::set<MWWorld::Ptr> mActors;

        public:

            Actors (MWWorld::Environment& environment);

            void addActor (const MWWorld::Ptr& ptr);
            ///< Register an actor for stats management

            void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management

            void dropActors (const MWWorld::Ptr::CellStore *cellStore);
            ///< Deregister all actors in the given cell.

            void update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement);
            ///< Update actor stats and store desired velocity vectors in \a movement
    };
}

#endif
