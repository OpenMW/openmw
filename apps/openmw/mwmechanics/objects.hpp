#ifndef GAME_MWMECHANICS_ACTIVATORS_H
#define GAME_MWMECHANICS_ACTIVATORS_H

#include "character.hpp"

#include <string>
#include <map>
#include <vector>
#include <list>

namespace osg
{
    class Vec3f;
}

namespace MWWorld
{
    class Ptr;
    class CellStore;
}

namespace MWMechanics
{
    class Objects
    {
        std::list<CharacterController> mObjects;
        std::map<const MWWorld::LiveCellRefBase*, std::list<CharacterController>::iterator> mIndex;

    public:
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

        bool onOpen(const MWWorld::Ptr& ptr);
        void onClose(const MWWorld::Ptr& ptr);

        bool playAnimationGroup(const MWWorld::Ptr& ptr, const std::string& groupName, int mode, int number, bool persist=false);
        void skipAnimation(const MWWorld::Ptr& ptr);
        void persistAnimationStates();

        void getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out) const;

        std::size_t size() const
        {
            return mObjects.size();
        }
    };
}

#endif
