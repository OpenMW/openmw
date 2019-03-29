#include "bulletshape.hpp"

#include <stdexcept>
#include <string>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

namespace Resource
{

BulletShape::BulletShape()
    : mCollisionShape(nullptr)
    , mAvoidCollisionShape(nullptr)
{

}

BulletShape::BulletShape(const BulletShape &copy, const osg::CopyOp &copyop)
    : mCollisionShape(duplicateCollisionShape(copy.mCollisionShape))
    , mAvoidCollisionShape(duplicateCollisionShape(copy.mAvoidCollisionShape))
    , mCollisionBoxHalfExtents(copy.mCollisionBoxHalfExtents)
    , mCollisionBoxTranslate(copy.mCollisionBoxTranslate)
    , mAnimatedShapes(copy.mAnimatedShapes)
{
}

BulletShape::~BulletShape()
{
    deleteShape(mAvoidCollisionShape);
    deleteShape(mCollisionShape);
}

void BulletShape::deleteShape(btCollisionShape* shape)
{
    if(shape!=nullptr)
    {
        if(shape->isCompound())
        {
            btCompoundShape* ms = static_cast<btCompoundShape*>(shape);
            int a = ms->getNumChildShapes();
            for(int i=0; i <a;i++)
                deleteShape(ms->getChildShape(i));
        }
        delete shape;
    }
}

btCollisionShape* BulletShape::duplicateCollisionShape(const btCollisionShape *shape) const
{
    if(shape->isCompound())
    {
        const btCompoundShape *comp = static_cast<const btCompoundShape*>(shape);
        btCompoundShape *newShape = new btCompoundShape;

        int numShapes = comp->getNumChildShapes();
        for(int i = 0;i < numShapes;++i)
        {
            btCollisionShape *child = duplicateCollisionShape(comp->getChildShape(i));
            btTransform trans = comp->getChildTransform(i);
            newShape->addChildShape(trans, child);
        }

        return newShape;
    }

    if(const btBvhTriangleMeshShape* trishape = dynamic_cast<const btBvhTriangleMeshShape*>(shape))
    {
        btScaledBvhTriangleMeshShape* newShape = new btScaledBvhTriangleMeshShape(const_cast<btBvhTriangleMeshShape*>(trishape), btVector3(1.f, 1.f, 1.f));
        return newShape;
    }

    if (const btBoxShape* boxshape = dynamic_cast<const btBoxShape*>(shape))
    {
        return new btBoxShape(*boxshape);
    }

    throw std::logic_error(std::string("Unhandled Bullet shape duplication: ")+shape->getName());
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

osg::ref_ptr<BulletShapeInstance> BulletShape::makeInstance() const
{
    osg::ref_ptr<BulletShapeInstance> instance (new BulletShapeInstance(this));
    return instance;
}

BulletShapeInstance::BulletShapeInstance(osg::ref_ptr<const BulletShape> source)
    : BulletShape()
    , mSource(source)
{
    mCollisionBoxHalfExtents = source->mCollisionBoxHalfExtents;
    mCollisionBoxTranslate = source->mCollisionBoxTranslate;

    mAnimatedShapes = source->mAnimatedShapes;

    if (source->mCollisionShape)
        mCollisionShape = duplicateCollisionShape(source->mCollisionShape);

    if (source->mAvoidCollisionShape)
        mAvoidCollisionShape = duplicateCollisionShape(source->mAvoidCollisionShape);
}

}
