#include "heightfield.hpp"

#include <osg/Object>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <LinearMath/btTransform.h>

namespace MWPhysics
{
    HeightField::HeightField(const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject)
    {
        mShape = new btHeightfieldTerrainShape(
            sqrtVerts, sqrtVerts, heights, 1,
            minH, maxH, 2,
            PHY_FLOAT, false
        );
        mShape->setUseDiamondSubdivision(true);
        mShape->setLocalScaling(btVector3(triSize, triSize, 1));

        btTransform transform(btQuaternion::getIdentity(),
                                btVector3((x+0.5f) * triSize * (sqrtVerts-1),
                                          (y+0.5f) * triSize * (sqrtVerts-1),
                                          (maxH+minH)*0.5f));

        mCollisionObject = new btCollisionObject;
        mCollisionObject->setCollisionShape(mShape);
        mCollisionObject->setWorldTransform(transform);

        mHoldObject = holdObject;
    }

    HeightField::~HeightField()
    {
        delete mCollisionObject;
        delete mShape;
    }

    btCollisionObject* HeightField::getCollisionObject()
    {
        return mCollisionObject;
    }

    const btCollisionObject* HeightField::getCollisionObject() const
    {
        return mCollisionObject;
    }

    const btHeightfieldTerrainShape* HeightField::getShape() const
    {
        return mShape;
    }
}
