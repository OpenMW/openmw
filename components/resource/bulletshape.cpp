#include "bulletshape.hpp"

#include <stdexcept>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

namespace Resource
{

BulletShape::BulletShape()
    : mCollisionShape(NULL)
{

}

BulletShape::~BulletShape()
{
    deleteShape(mCollisionShape);
}

void BulletShape::deleteShape(btCollisionShape* shape)
{
    if(shape!=NULL)
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

btCollisionShape* BulletShape::duplicateCollisionShape(btCollisionShape *shape) const
{
    if(shape->isCompound())
    {
        btCompoundShape *comp = static_cast<btCompoundShape*>(shape);
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

    if(btBvhTriangleMeshShape* trishape = dynamic_cast<btBvhTriangleMeshShape*>(shape))
    {
#if BT_BULLET_VERSION >= 283
        btScaledBvhTriangleMeshShape* newShape = new btScaledBvhTriangleMeshShape(trishape, btVector3(1.f, 1.f, 1.f));
#else
        // work around btScaledBvhTriangleMeshShape bug ( https://code.google.com/p/bullet/issues/detail?id=371 ) in older bullet versions
        btTriangleMesh* oldMesh = static_cast<btTriangleMesh*>(trishape->getMeshInterface());
        btTriangleMesh* newMesh = new btTriangleMesh(*oldMesh);

        // Do not build a new bvh (not needed, since it's the same as the original shape's bvh)
        bool buildBvh = true;
        if (trishape->getOptimizedBvh())
            buildBvh = false;
        TriangleMeshShape* newShape = new TriangleMeshShape(newMesh, true, buildBvh);
        // Set original shape's bvh via pointer
        // The pointer is safe because the BulletShapeInstance keeps a ref_ptr to the original BulletShape
        if (!buildBvh)
            newShape->setOptimizedBvh(trishape->getOptimizedBvh());
#endif
        return newShape;
    }

    if (btBoxShape* boxshape = dynamic_cast<btBoxShape*>(shape))
    {
        return new btBoxShape(*boxshape);
    }

    throw std::logic_error(std::string("Unhandled Bullet shape duplication: ")+shape->getName());
}

btCollisionShape *BulletShape::getCollisionShape()
{
    return mCollisionShape;
}

osg::ref_ptr<BulletShapeInstance> BulletShape::makeInstance()
{
    osg::ref_ptr<BulletShapeInstance> instance (new BulletShapeInstance(this));
    return instance;
}

BulletShapeInstance::BulletShapeInstance(osg::ref_ptr<BulletShape> source)
    : BulletShape()
    , mSource(source)
{
    mCollisionBoxHalfExtents = source->mCollisionBoxHalfExtents;
    mCollisionBoxTranslate = source->mCollisionBoxTranslate;

    mAnimatedShapes = source->mAnimatedShapes;

    if (source->mCollisionShape)
        mCollisionShape = duplicateCollisionShape(source->mCollisionShape);
}

}
