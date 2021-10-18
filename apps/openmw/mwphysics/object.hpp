#ifndef OPENMW_MWPHYSICS_OBJECT_H
#define OPENMW_MWPHYSICS_OBJECT_H

#include "ptrholder.hpp"

#include <LinearMath/btTransform.h>
#include <osg/Node>

#include <map>
#include <memory>
#include <mutex>

namespace Resource
{
    class BulletShapeInstance;
}

class btCollisionObject;
class btVector3;

namespace MWPhysics
{
    class PhysicsTaskScheduler;

    class Object final : public PtrHolder
    {
    public:
        Object(const MWWorld::Ptr& ptr, osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance, osg::Quat rotation, int collisionType, PhysicsTaskScheduler* scheduler);
        ~Object() override;

        const Resource::BulletShapeInstance* getShapeInstance() const;
        void setScale(float scale);
        void setRotation(osg::Quat quat);
        void updatePosition();
        void commitPositionChange();
        btTransform getTransform() const;
        /// Return solid flag. Not used by the object itself, true by default.
        bool isSolid() const;
        void setSolid(bool solid);
        bool isAnimated() const;
        /// @brief update object shape
        /// @return true if shape changed
        bool animateCollisionShapes();

    private:
        osg::ref_ptr<Resource::BulletShapeInstance> mShapeInstance;
        std::map<int, osg::NodePath> mRecIndexToNodePath;
        bool mSolid;
        btVector3 mScale;
        osg::Vec3f mPosition;
        osg::Quat mRotation;
        bool mScaleUpdatePending = false;
        bool mTransformUpdatePending = false;
        mutable std::mutex mPositionMutex;
        PhysicsTaskScheduler* mTaskScheduler;
    };
}

#endif
