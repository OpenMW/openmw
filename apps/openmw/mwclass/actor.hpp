#ifndef GAME_MWCLASS_MOBILE_H
#define GAME_MWCLASS_MOBILE_H

#include "../mwworld/class.hpp"

#include <components/esm3/loadmgef.hpp>

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

        Actor() = default;

        template <class GMST>
        float getSwimSpeedImpl(const MWWorld::Ptr& ptr, const GMST& gmst, const MWMechanics::MagicEffects& mageffects, float baseSpeed) const
        {
            return baseSpeed
                * (1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).getMagnitude())
                * (gmst.fSwimRunBase->mValue.getFloat()
                   + 0.01f * getSkill(ptr, ESM::Skill::Athletics) * gmst.fSwimRunAthleticsMult->mValue.getFloat());
        }

    public:
         ~Actor() override = default;

        void adjustPosition(const MWWorld::Ptr& ptr, bool force) const override;
        ///< Adjust position to stand on ground. Must be called post model load
        /// @param force do this even if the ptr is flying

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation, MWPhysics::PhysicsSystem& physics) const override;

        bool useAnim() const override;

        void block(const MWWorld::Ptr &ptr) const override;

        osg::Vec3f getRotationVector(const MWWorld::Ptr& ptr) const override;
        ///< Return desired rotations, as euler angles. Sets getMovementSettings(ptr).mRotation to zero.

        float getEncumbrance(const MWWorld::Ptr& ptr) const override;
        ///< Returns total weight of objects inside this object (including modifications from magic
        /// effects). Throws an exception, if the object can't hold other objects.

        bool allowTelekinesis(const MWWorld::ConstPtr& ptr) const override;
        ///< Return whether this class of object can be activated with telekinesis

        bool isActor() const override;

        /// Return current movement speed.
        float getCurrentSpeed(const MWWorld::Ptr& ptr) const override;
        
        // not implemented
        Actor(const Actor&) = delete;
        Actor& operator= (const Actor&) = delete;
    };
}

#endif
