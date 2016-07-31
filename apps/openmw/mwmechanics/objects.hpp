#ifndef GAME_MWMECHANICS_ACTIVATORS_H
#define GAME_MWMECHANICS_ACTIVATORS_H

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
    class Objects
    {
        typedef std::map<MWWorld::Ptr,CharacterController*> PtrControllerMap;
        PtrControllerMap mObjects;

    public:
        Objects();
        ~Objects();

        void addObject (const MWWorld::Ptr& ptr);
        ///< Register an animated object

        void removeObject (const MWWorld::Ptr& ptr);
        ///< Deregister an object

        void updateObject(const MWWorld::Ptr &old, const MWWorld::Ptr& ptr);
        ///< Updates an object with a new Ptr

        void dropObjects(const MWWorld::CellStore *cellStore);
        ///< Deregister all objects in the given cell.

        void update(float duration, bool paused);
        ///< Update object animations

        bool playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number);
        void skipAnimation(const MWWorld::Ptr& ptr);

        void getObjectsInRange (const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out);
    };
}

#endif
