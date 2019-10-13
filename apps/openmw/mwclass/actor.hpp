#ifndef GAME_MWCLASS_MOBILE_H
#define GAME_MWCLASS_MOBILE_H

#include "../mwworld/class.hpp"

namespace ESM
{
    struct GameSetting;
}

namespace MWClass
{
    /// \brief Class holding functionality common to Creature and NPC
    class Actor : public MWWorld::Class
    {
    protected:

        Actor();

    public:
        virtual ~Actor();

        virtual void adjustPosition(const MWWorld::Ptr& ptr, bool force) const;
        ///< Adjust position to stand on ground. Must be called post model load
        /// @param force do this even if the ptr is flying

        virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const;

        virtual bool useAnim() const;

        virtual void block(const MWWorld::Ptr &ptr) const;

        virtual osg::Vec3f getRotationVector(const MWWorld::Ptr& ptr) const;
        ///< Return desired rotations, as euler angles.

        virtual float getEncumbrance(const MWWorld::Ptr& ptr) const;
        ///< Returns total weight of objects inside this object (including modifications from magic
        /// effects). Throws an exception, if the object can't hold other objects.

        virtual bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const;
        ///< Return whether this class of object can be activated with telekinesis

        virtual bool isActor() const;

        // not implemented
        Actor(const Actor&);
        Actor& operator= (const Actor&);
    };
}

#endif
