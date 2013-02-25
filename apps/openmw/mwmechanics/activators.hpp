#ifndef GAME_MWMECHANICS_ACTIVATORS_H
#define GAME_MWMECHANICS_ACTOVATRS_H

#include <string>
#include <map>

#include "character.hpp"

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWMechanics
{
    class Activators
    {
        typedef std::map<MWWorld::Ptr,CharacterController> PtrControllerMap;
        PtrControllerMap mActivators;

    public:
        Activators();

        void addActivator (const MWWorld::Ptr& ptr);
        ///< Register an animated activator

        void removeActivator (const MWWorld::Ptr& ptr);
        ///< Deregister an activator

        void updateActivator(const MWWorld::Ptr &old, const MWWorld::Ptr& ptr);
        ///< Updates an activator with a new Ptr

        void dropActivators (const MWWorld::CellStore *cellStore);
        ///< Deregister all activators in the given cell.

        void update (float duration, bool paused);
        ///< Update activator animations

        void playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
        void skipAnimation(const MWWorld::Ptr& ptr);
    };
}

#endif
