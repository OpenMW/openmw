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

namespace MWMechanics
{
    class Actors
    {
            std::set<MWWorld::Ptr> mActors;
            float mDuration;

            void updateActor (const MWWorld::Ptr& ptr, float duration);

            void updateNpc (const MWWorld::Ptr& ptr, float duration, bool paused);

            void adjustMagicEffects (const MWWorld::Ptr& creature);

        public:

            Actors();

            void addActor (const MWWorld::Ptr& ptr);
            ///< Register an actor for stats management

            void removeActor (const MWWorld::Ptr& ptr);
            ///< Deregister an actor for stats management

            void dropActors (const MWWorld::Ptr::CellStore *cellStore);
            ///< Deregister all actors in the given cell.

            void update (std::vector<std::pair<std::string, Ogre::Vector3> >& movement,
                float duration, bool paused);
            ///< Update actor stats and store desired velocity vectors in \a movement
    };
}

#endif
