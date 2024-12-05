#include "bulletshape.hpp"

#include <stdexcept>
#include <string>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>

namespace Resource
{
    namespace
    {
        CollisionShapePtr duplicateCollisionShape(const btCollisionShape* shape)
        {
            if (shape == nullptr)
                return nullptr;

            if (shape->isCompound())
            {
                const btCompoundShape* comp = static_cast<const btCompoundShape*>(shape);
                std::unique_ptr<btCompoundShape, DeleteCollisionShape> newShape(new btCompoundShape);

                for (int i = 0, n = comp->getNumChildShapes(); i < n; ++i)
                {
                    auto child = duplicateCollisionShape(comp->getChildShape(i));
                    const btTransform& trans = comp->getChildTransform(i);
                    newShape->addChildShape(trans, child.release());
                }

                return newShape;
            }

            if (shape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
            {
                const btScaledBvhTriangleMeshShape* trishape = static_cast<const btScaledBvhTriangleMeshShape*>(shape);
                return CollisionShapePtr(new btScaledBvhTriangleMeshShape(
                    const_cast<btBvhTriangleMeshShape*>(trishape->getChildShape()), trishape->getLocalScaling()));
            }

            if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
            {
                const btBvhTriangleMeshShape* trishape = static_cast<const btBvhTriangleMeshShape*>(shape);
                return CollisionShapePtr(new btScaledBvhTriangleMeshShape(
                    const_cast<btBvhTriangleMeshShape*>(trishape), btVector3(1.f, 1.f, 1.f)));
            }

            if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
            {
                const btBoxShape* boxshape = static_cast<const btBoxShape*>(shape);
                return CollisionShapePtr(new btBoxShape(*boxshape));
            }

            if (shape->getShapeType() == TERRAIN_SHAPE_PROXYTYPE)
                return CollisionShapePtr(
                    new btHeightfieldTerrainShape(static_cast<const btHeightfieldTerrainShape&>(*shape)));

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

    void DeleteCollisionShape::operator()(btCollisionShape* shape) const
    {
        deleteShape(shape);
    }

    BulletShape::BulletShape(const BulletShape& other, const osg::CopyOp& copyOp)
        : Object(other, copyOp)
        , mCollisionShape(duplicateCollisionShape(other.mCollisionShape.get()))
        , mAvoidCollisionShape(duplicateCollisionShape(other.mAvoidCollisionShape.get()))
        , mCollisionBox(other.mCollisionBox)
        , mAnimatedShapes(other.mAnimatedShapes)
        , mFileName(other.mFileName)
        , mFileHash(other.mFileHash)
        , mVisualCollisionType(other.mVisualCollisionType)
    {
    }

    void BulletShape::setLocalScaling(const btVector3& scale)
    {
        mCollisionShape->setLocalScaling(scale);
        if (mAvoidCollisionShape)
            mAvoidCollisionShape->setLocalScaling(scale);
    }

    osg::ref_ptr<BulletShapeInstance> makeInstance(osg::ref_ptr<const BulletShape> source)
    {
        return { new BulletShapeInstance(std::move(source)) };
    }

    BulletShapeInstance::BulletShapeInstance(osg::ref_ptr<const BulletShape> source)
        : BulletShape(*source)
        , mSource(std::move(source))
    {
    }
}
