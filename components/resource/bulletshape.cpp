#include "bulletshape.hpp"

#include <stdexcept>
#include <string>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace Resource
{
namespace
{
    btCollisionShape* duplicateCollisionShape(const btCollisionShape *shape)
    {
        if (shape == nullptr)
            return nullptr;

        if (shape->isCompound())
        {
            const btCompoundShape *comp = static_cast<const btCompoundShape*>(shape);
            btCompoundShape* newShape = new btCompoundShape;

            for (int i = 0, n = comp->getNumChildShapes(); i < n; ++i)
            {
                btCollisionShape* child = duplicateCollisionShape(comp->getChildShape(i));
                const btTransform& trans = comp->getChildTransform(i);
                newShape->addChildShape(trans, child);
            }

            return newShape;
        }

        if (const btBvhTriangleMeshShape* trishape = dynamic_cast<const btBvhTriangleMeshShape*>(shape))
            return new btScaledBvhTriangleMeshShape(const_cast<btBvhTriangleMeshShape*>(trishape), btVector3(1.f, 1.f, 1.f));

        if (const btBoxShape* boxshape = dynamic_cast<const btBoxShape*>(shape))
            return new btBoxShape(*boxshape);

        if (shape->getShapeType() == TERRAIN_SHAPE_PROXYTYPE)
            return new btHeightfieldTerrainShape(static_cast<const btHeightfieldTerrainShape&>(*shape));

        throw std::logic_error(std::string("Unhandled Bullet shape duplication: ") + shape->getName());
    }

    void deleteShape(btCollisionShape* shape)
    {
        if (shape->isCompound())
        {
            btCompoundShape* compound = static_cast<btCompoundShape*>(shape);
            for (int i = 0, n = compound->getNumChildShapes(); i < n; i++)
                if (btCollisionShape* child = compound->getChildShape(i))
                    deleteShape(child);
        }

        delete shape;
    }
}

BulletShape::BulletShape()
    : mCollisionShape(nullptr)
    , mAvoidCollisionShape(nullptr)
{

}

BulletShape::BulletShape(const BulletShape &copy, const osg::CopyOp &copyop)
    : mCollisionShape(duplicateCollisionShape(copy.mCollisionShape))
    , mAvoidCollisionShape(duplicateCollisionShape(copy.mAvoidCollisionShape))
    , mCollisionBox(copy.mCollisionBox)
    , mAnimatedShapes(copy.mAnimatedShapes)
{
}

BulletShape::~BulletShape()
{
    if (mAvoidCollisionShape != nullptr)
        deleteShape(mAvoidCollisionShape);
    if (mCollisionShape != nullptr)
        deleteShape(mCollisionShape);
}

btCollisionShape *BulletShape::getCollisionShape() const
{
    return mCollisionShape;
}

btCollisionShape *BulletShape::getAvoidCollisionShape() const
{
    return mAvoidCollisionShape;
}

void BulletShape::setLocalScaling(const btVector3& scale)
{
    mCollisionShape->setLocalScaling(scale);
    if (mAvoidCollisionShape)
        mAvoidCollisionShape->setLocalScaling(scale);
}

bool BulletShape::isAnimated() const
{
    return !mAnimatedShapes.empty();
}

osg::ref_ptr<BulletShapeInstance> BulletShape::makeInstance() const
{
    osg::ref_ptr<BulletShapeInstance> instance (new BulletShapeInstance(this));
    return instance;
}

BulletShapeInstance::BulletShapeInstance(osg::ref_ptr<const BulletShape> source)
    : BulletShape()
    , mSource(source)
{
    mCollisionBox = source->mCollisionBox;
    mAnimatedShapes = source->mAnimatedShapes;
    mCollisionShape = duplicateCollisionShape(source->mCollisionShape);
    mAvoidCollisionShape = duplicateCollisionShape(source->mAvoidCollisionShape);
}

}
