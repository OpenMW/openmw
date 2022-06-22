#include "heightfield.hpp"
#include "mtphysics.hpp"

#include <components/bullethelpers/heightfield.hpp>

#include <osg/Object>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <LinearMath/btTransform.h>

#include <type_traits>
#include <concepts>

// Older Bullet versions only support `btScalar` heightfields.
// Our heightfield data is `float`.
//
// These functions handle conversion from `float` to `double` when
// `btScalar` is `double` (`BT_USE_DOUBLE_PRECISION`).
namespace
{
    template <std::same_as<btScalar> T>
    std::vector<btScalar> makeHeights(const T* /*heights*/, int /*verts*/)
    {
        return {};
    }

    template <class T>
    std::vector<btScalar> makeHeights(const T* heights, int verts)
        requires (!std::is_same_v<btScalar, T>)
    {
        return std::vector<btScalar>(heights, heights + static_cast<std::ptrdiff_t>(verts * verts));
    }

    template <std::same_as<btScalar> T>
    const btScalar* getHeights(const T* floatHeights, const std::vector<btScalar>&)
    {
        return floatHeights;
    }

    template <class T>
        requires (!std::is_same_v<btScalar, T>)
    const btScalar* getHeights(const T*, const std::vector<btScalar>& btScalarHeights)
    {
        return btScalarHeights.data();
    }
}

namespace MWPhysics
{
    HeightField::HeightField(const float* heights, int x, int y, int size, int verts, float minH, float maxH,
                             const osg::Object* holdObject, PhysicsTaskScheduler* scheduler)
        : mHoldObject(holdObject)
#if BT_BULLET_VERSION < 310
        , mHeights(makeHeights(heights, verts))
#endif
        , mTaskScheduler(scheduler)
    {
#if BT_BULLET_VERSION < 310
        mShape = std::make_unique<btHeightfieldTerrainShape>(
            verts, verts,
            getHeights(heights, mHeights),
            1,
            minH, maxH, 2,
            PHY_FLOAT, false
        );
#else
        mShape = std::make_unique<btHeightfieldTerrainShape>(
            verts, verts, heights, minH, maxH, 2, false);
#endif
        mShape->setUseDiamondSubdivision(true);

        const float scaling = static_cast<float>(size) / static_cast<float>(verts - 1);
        mShape->setLocalScaling(btVector3(scaling, scaling, 1));

#if BT_BULLET_VERSION >= 289
        // Accelerates some collision tests.
        //
        // Note: The accelerator data structure in Bullet is only used
        // in some operations. This could be improved, see:
        // https://github.com/bulletphysics/bullet3/issues/3276
        mShape->buildAccelerator();
#endif

        const btTransform transform(btQuaternion::getIdentity(),
                                    BulletHelpers::getHeightfieldShift(x, y, size, minH, maxH));

        mCollisionObject = std::make_unique<btCollisionObject>();
        mCollisionObject->setCollisionShape(mShape.get());
        mCollisionObject->setWorldTransform(transform);
        mTaskScheduler->addCollisionObject(mCollisionObject.get(), CollisionType_HeightMap, CollisionType_Actor|CollisionType_Projectile);
    }

    HeightField::~HeightField()
    {
        mTaskScheduler->removeCollisionObject(mCollisionObject.get());
    }

    btCollisionObject* HeightField::getCollisionObject()
    {
        return mCollisionObject.get();
    }

    const btCollisionObject* HeightField::getCollisionObject() const
    {
        return mCollisionObject.get();
    }

    const btHeightfieldTerrainShape* HeightField::getShape() const
    {
        return mShape.get();
    }
}
