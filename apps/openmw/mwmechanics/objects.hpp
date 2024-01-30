#ifndef GAME_MWMECHANICS_ACTIVATORS_H
#define GAME_MWMECHANICS_ACTIVATORS_H

#include "character.hpp"

#include <list>
#include <map>
#include <string>
#include <vector>

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
        void addObject(const MWWorld::Ptr& ptr);
        ///< Register an animated object

        void removeObject(const MWWorld::Ptr& ptr);
        ///< Deregister an object

        void updateObject(const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);
        ///< Updates an object with a new Ptr

        void dropObjects(const MWWorld::CellStore* cellStore);
        ///< Deregister all objects in the given cell.

        void update(float duration, bool paused);
        ///< Update object animations

        bool onOpen(const MWWorld::Ptr& ptr);
        void onClose(const MWWorld::Ptr& ptr);

        bool playAnimationGroup(
            const MWWorld::Ptr& ptr, std::string_view groupName, int mode, uint32_t number, bool scripted = false);
        bool playAnimationGroupLua(const MWWorld::Ptr& ptr, std::string_view groupName, uint32_t loops, float speed,
            std::string_view startKey, std::string_view stopKey, bool forceLoop);
        void enableLuaAnimations(const MWWorld::Ptr& ptr, bool enable);
        void skipAnimation(const MWWorld::Ptr& ptr);
        void persistAnimationStates();
        void clearAnimationQueue(const MWWorld::Ptr& ptr, bool clearScripted);

        void getObjectsInRange(const osg::Vec3f& position, float radius, std::vector<MWWorld::Ptr>& out) const;

        std::size_t size() const { return mObjects.size(); }
    };
}

#endif
