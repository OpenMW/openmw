#include "object.hpp"
#include "mtphysics.hpp"

#include <components/debug/debuglog.hpp>
#include <components/nifosg/particle.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <LinearMath/btTransform.h>

namespace MWPhysics
{
    Object::Object(const MWWorld::Ptr& ptr, osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance, PhysicsTaskScheduler* scheduler)
        : mShapeInstance(shapeInstance)
        , mSolid(true)
        , mTaskScheduler(scheduler)
    {
        mPtr = ptr;

        mCollisionObject.reset(new btCollisionObject);
        mCollisionObject->setCollisionShape(shapeInstance->getCollisionShape());

        mCollisionObject->setUserPointer(this);

        setScale(ptr.getCellRef().getScale());
        setRotation(Misc::Convert::toBullet(ptr.getRefData().getBaseNode()->getAttitude()));
        const float* pos = ptr.getRefData().getPosition().pos;
        setOrigin(btVector3(pos[0], pos[1], pos[2]));
        commitPositionChange();
    }

    Object::~Object()
    {
        if (mCollisionObject)
            mTaskScheduler->removeCollisionObject(mCollisionObject.get());
    }

    const Resource::BulletShapeInstance* Object::getShapeInstance() const
    {
        return mShapeInstance.get();
    }

    void Object::setScale(float scale)
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mScale = { scale,scale,scale };
        mScaleUpdatePending = true;
    }

    void Object::setRotation(const btQuaternion& quat)
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mLocalTransform.setRotation(quat);
        mTransformUpdatePending = true;
    }

    void Object::setOrigin(const btVector3& vec)
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        mLocalTransform.setOrigin(vec);
        mTransformUpdatePending = true;
    }

    void Object::commitPositionChange()
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        if (mScaleUpdatePending)
        {
            mShapeInstance->setLocalScaling(mScale);
            mScaleUpdatePending = false;
        }
        if (mTransformUpdatePending)
        {
            mCollisionObject->setWorldTransform(mLocalTransform);
            mTransformUpdatePending = false;
        }
    }

    btCollisionObject* Object::getCollisionObject()
    {
        return mCollisionObject.get();
    }

    const btCollisionObject* Object::getCollisionObject() const
    {
        return mCollisionObject.get();
    }

    btTransform Object::getTransform() const
    {
        std::unique_lock<std::mutex> lock(mPositionMutex);
        return mLocalTransform;
    }

    bool Object::isSolid() const
    {
        return mSolid;
    }

    void Object::setSolid(bool solid)
    {
        mSolid = solid;
    }

    bool Object::isAnimated() const
    {
        return !mShapeInstance->mAnimatedShapes.empty();
    }

    bool Object::animateCollisionShapes()
    {
        if (mShapeInstance->mAnimatedShapes.empty())
            return false;

        assert (mShapeInstance->getCollisionShape()->isCompound());

        btCompoundShape* compound = static_cast<btCompoundShape*>(mShapeInstance->getCollisionShape());
        for (const auto& shape : mShapeInstance->mAnimatedShapes)
        {
            int recIndex = shape.first;
            int shapeIndex = shape.second;

            auto nodePathFound = mRecIndexToNodePath.find(recIndex);
            if (nodePathFound == mRecIndexToNodePath.end())
            {
                NifOsg::FindGroupByRecIndex visitor(recIndex);
                mPtr.getRefData().getBaseNode()->accept(visitor);
                if (!visitor.mFound)
                {
                    Log(Debug::Warning) << "Warning: animateCollisionShapes can't find node " << recIndex << " for " << mPtr.getCellRef().getRefId();

                    // Remove nonexistent nodes from animated shapes map and early out
                    mShapeInstance->mAnimatedShapes.erase(recIndex);
                    return false;
                }
                osg::NodePath nodePath = visitor.mFoundPath;
                nodePath.erase(nodePath.begin());
                nodePathFound = mRecIndexToNodePath.emplace(recIndex, nodePath).first;
            }

            osg::NodePath& nodePath = nodePathFound->second;
            osg::Matrixf matrix = osg::computeLocalToWorld(nodePath);
            matrix.orthoNormalize(matrix);

            btTransform transform;
            transform.setOrigin(Misc::Convert::toBullet(matrix.getTrans()) * compound->getLocalScaling());
            for (int i=0; i<3; ++i)
                for (int j=0; j<3; ++j)
                    transform.getBasis()[i][j] = matrix(j,i); // NB column/row major difference

            // Note: we can not apply scaling here for now since we treat scaled shapes
            // as new shapes (btScaledBvhTriangleMeshShape) with 1.0 scale for now
            if (!(transform == compound->getChildTransform(shapeIndex)))
                compound->updateChildTransform(shapeIndex, transform);
        }
        return true;
    }
}
