#ifndef OPENMW_MWPHYSICS_OBJECT_H
#define OPENMW_MWPHYSICS_OBJECT_H

#include "ptrholder.hpp"

#include <osg/Node>

#include <map>
#include <memory>

namespace Resource
{
    class BulletShapeInstance;
}

class btCollisionObject;
class btCollisionWorld;
class btQuaternion;
class btVector3;

namespace MWPhysics
{
    class Object : public PtrHolder
    {
    public:
        Object(const MWWorld::Ptr& ptr, osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance);

        const Resource::BulletShapeInstance* getShapeInstance() const;
        void setScale(float scale);
        void setRotation(const btQuaternion& quat);
        void setOrigin(const btVector3& vec);
        btCollisionObject* getCollisionObject();
        const btCollisionObject* getCollisionObject() const;
        /// Return solid flag. Not used by the object itself, true by default.
        bool isSolid() const;
        void setSolid(bool solid);
        bool isAnimated() const;
        void animateCollisionShapes(btCollisionWorld* collisionWorld);

    private:
        std::unique_ptr<btCollisionObject> mCollisionObject;
        osg::ref_ptr<Resource::BulletShapeInstance> mShapeInstance;
        std::map<int, osg::NodePath> mRecIndexToNodePath;
        bool mSolid;
    };
}

#endif
