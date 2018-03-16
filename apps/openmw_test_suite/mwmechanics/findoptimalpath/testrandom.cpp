#include "collisionobjects.hpp"
#include "matchers.hpp"

#include "apps/openmw/mwmechanics/findoptimalpath.hpp"

#include "apps/openmw/mwphysics/closestcollision.hpp"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>
#include <BulletCollision/CollisionShapes/btPolyhedralConvexShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <osg/Math>

#include <boost/optional.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef FIND_OPTIMAL_PATH_JSON
#include <extern/nlohmann/json.hpp>
#endif

inline std::ostream& operator <<(std::ostream& stream, const btVector3& value)
{
    return stream << std::setprecision(std::numeric_limits<btScalar>::digits)
        << "(" << value.x() << ", " << value.y() << ", " << value.z() << ")";
}

namespace
{

    using namespace testing;
    using namespace MWMechanics;
    using MWPhysics::getClosestCollision;

    struct FindOptimalPathWithRandomGeneratedWorldTest : Test
    {
        Floor floor;
        Actor actor;
        btDefaultCollisionConfiguration collisionConfiguration;
        btCollisionDispatcher dispatcher {&collisionConfiguration};
        btDbvtBroadphase broadphase;
        btCollisionWorld collisionWorld {&dispatcher, &broadphase, &collisionConfiguration};
        FindOptimalPathConfig config;

        FindOptimalPathWithRandomGeneratedWorldTest()
        {
            config.mAllowFly = false;
            config.mActorHalfExtents = actor.halfExtents;
            config.mMaxDepth = std::numeric_limits<std::size_t>::max();
            config.mMaxIterations = std::numeric_limits<std::size_t>::max();
            config.mHasNearCollisionFilterFactor = 0.33f;
            config.mHasNearCollisionFilterReduceFactor = 0.5f;
            config.mHorizontalMarginFactor = 2;

            collisionWorld.addCollisionObject(&actor.object);
            collisionWorld.addCollisionObject(&floor.object);
            collisionWorld.updateAabbs();
        }

        btScalar getDistanceToGround(const btVector3& point) const
        {
            if (const auto collision = getClosestCollision(actor.object, point, point - btVector3(0, 0, 1e5), collisionWorld))
            {
                return point.distance(collision->mPoint);
            }
            else
            {
                return std::numeric_limits<btScalar>::max();
            }
        }

        void assertThatAllPathIsOnGround(btVector3 initial, const std::vector<btVector3>& points)
        {
            ASSERT_THAT(initial, IsOnGround(this));
            for (const auto& point : points)
            {
                ASSERT_THAT(point, IsOnGround(this));
//s                ASSERT_THAT(Transition(initial, point), MedianPointIsOnGround(this));
                initial = point;
            }
        }

        void assertThatNoCollisionsOnThePath(btVector3 initial, const std::vector<btVector3>& points)
        {
            for (const auto& point : points)
            {
                ASSERT_THAT(Transition(initial, point), IsClear(this));
                initial = point;
            }
        }

        btVector3 onGround(const btVector3& point)
        {
            const auto collision = getClosestCollision(actor.object, point, point - btVector3(0, 0, 1e5), collisionWorld);
            return collision ? collision->mEnd + btVector3(0, 0, 1e-2f) : point;
        }
    };

    TEST_F(FindOptimalPathWithRandomGeneratedWorldTest, with_floor_and_random_spheres)
    {
        const btVector3 initial = onGround(actor.origin - btVector3(200, 0, 0));
        const btVector3 goal = onGround(actor.origin + btVector3(2000, 0, 0));

        std::mt19937_64 device(0);
        std::uniform_real_distribution<btScalar> uniform(0, 1);
        std::exponential_distribution<btScalar> exponential(1);
        std::vector<std::unique_ptr<btCollisionShape>> shapes;
        std::vector<std::unique_ptr<btCollisionObject>> objects;
        for (std::size_t i = 0; i < 700; ++i)
        {
            const auto x = btScalar(4000) * uniform(device);
            const auto y = btScalar(10000) * (uniform(device) - btScalar(0.5));
            const auto radius = std::min(btScalar(3) * actor.halfExtents.z(), actor.halfExtents.z() * (btScalar(0.5) + exponential(device)));
            std::unique_ptr<btCollisionShape> shape(new btSphereShape(radius));
            shapes.push_back(std::move(shape));
            std::unique_ptr<btCollisionObject> object(new btCollisionObject);
            object->setCollisionShape(shapes.back().get());
            object->setWorldTransform(btTransform(btMatrix3x3::getIdentity(), btVector3(x, y, actor.halfExtents.z())));
            object->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            objects.push_back(std::move(object));
            collisionWorld.addCollisionObject(objects.back().get());
        }
        collisionWorld.updateAabbs();
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);

        for (const auto& object : objects)
        {
            collisionWorld.removeCollisionObject(object.get());
        }
    }

    TEST_F(FindOptimalPathWithRandomGeneratedWorldTest, with_floor_and_random_boxes)
    {
        const btVector3 initial = onGround(actor.origin - btVector3(200, 0, 0));
        const btVector3 goal = onGround(actor.origin + btVector3(2000, 0, 0));

        std::mt19937_64 device(100);
        std::uniform_real_distribution<btScalar> uniform(0, 1);
        std::exponential_distribution<btScalar> exponential(1);
        std::vector<std::unique_ptr<btCollisionShape>> shapes;
        std::vector<std::unique_ptr<btCollisionObject>> objects;
        for (std::size_t i = 0; i < 500; ++i)
        {
            const auto x = btScalar(4000) * uniform(device);
            const auto y = btScalar(10000) * (uniform(device) - btScalar(0.5));
            const auto halfExtent = std::min(btScalar(3) * actor.halfExtents.z(), actor.halfExtents.z() * (btScalar(0.5) + exponential(device)));
            std::unique_ptr<btCollisionShape> shape(new btBoxShape(btVector3(halfExtent, halfExtent, halfExtent)));
            shapes.push_back(std::move(shape));
            std::unique_ptr<btCollisionObject> object(new btCollisionObject);
            object->setCollisionShape(shapes.back().get());
            object->setWorldTransform(btTransform(btMatrix3x3::getIdentity(), btVector3(x, y, actor.halfExtents.z())));
            object->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            objects.push_back(std::move(object));
            collisionWorld.addCollisionObject(objects.back().get());
        }
        collisionWorld.updateAabbs();

        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);

        for (const auto& object : objects)
        {
            collisionWorld.removeCollisionObject(object.get());
        }
    }

    TEST_F(FindOptimalPathWithRandomGeneratedWorldTest, with_random_height_field)
    {
        const btScalar maxAllowedHeight = 32;

        std::mt19937_64 device(100);
        std::uniform_real_distribution<btScalar> uniform(0, maxAllowedHeight);

        const std::size_t heightStickWidth = 65;
        const std::size_t heightStickLength = 65;
        std::vector<btScalar> heightfieldData;
        std::generate_n(std::back_inserter(heightfieldData), heightStickWidth * heightStickLength,
                        [&] { return uniform(device); });
        const btScalar heightScale = 1;
        const btScalar minHeight = *std::min_element(heightfieldData.begin(), heightfieldData.end());
        const btScalar maxHeight = *std::max_element(heightfieldData.begin(), heightfieldData.end());
        const int upAxis = 2;
        const PHY_ScalarType heightDataType = PHY_FLOAT;
        const bool flipQuadEdges = false;
        btHeightfieldTerrainShape shape(
            heightStickWidth,
            heightStickLength,
            heightfieldData.data(),
            heightScale,
            minHeight,
            maxHeight,
            upAxis,
            heightDataType,
            flipQuadEdges
        );
        btCollisionObject object;
        shape.setLocalScaling(btVector3(128, 128, 1));
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)));
        object.setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
        collisionWorld.addCollisionObject(&object);
        collisionWorld.updateAabbs();

        const btVector3 initial = onGround(btVector3(-2000, 0, maxAllowedHeight + 100));
        const btVector3 goal = onGround(btVector3(2000, 0, maxAllowedHeight + 100));

        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);

        collisionWorld.removeCollisionObject(&object);
    }

    TEST_F(FindOptimalPathWithRandomGeneratedWorldTest, with_random_height_field_and_random_boxes)
    {
        const btScalar maxAllowedHeight = 32;

        std::mt19937_64 device(100);
        std::uniform_real_distribution<btScalar> uniform(0, 1);
        std::exponential_distribution<btScalar> exponential(1);
        std::vector<std::unique_ptr<btCollisionShape>> shapes;
        std::vector<std::unique_ptr<btCollisionObject>> objects;
        for (std::size_t i = 0; i < 100; ++i)
        {
            const auto x = btScalar(3600) * (uniform(device) - btScalar(0.5));
            const auto y = btScalar(10000) * (uniform(device) - btScalar(0.5));
            const auto halfExtent = std::min(btScalar(3) * actor.halfExtents.z(), actor.halfExtents.z() * (btScalar(0.5) + exponential(device)));
            std::unique_ptr<btCollisionShape> shape(new btBoxShape(btVector3(halfExtent, halfExtent, halfExtent)));
            shapes.push_back(std::move(shape));
            std::unique_ptr<btCollisionObject> object(new btCollisionObject);
            object->setCollisionShape(shapes.back().get());
            const auto position = onGround(btVector3(x, y, maxAllowedHeight) + btVector3(0, 0, actor.halfExtents.z()));
            object->setWorldTransform(btTransform(btMatrix3x3::getIdentity(), position));
            object->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            objects.push_back(std::move(object));
            collisionWorld.addCollisionObject(objects.back().get());
        }

        uniform = std::uniform_real_distribution<btScalar>(0, maxAllowedHeight);

        const std::size_t heightStickWidth = 65;
        const std::size_t heightStickLength = 65;
        std::vector<btScalar> heightfieldData;
        std::generate_n(std::back_inserter(heightfieldData), heightStickWidth * heightStickLength,
                        [&] { return uniform(device); });

        const btScalar heightScale = 1;
        const btScalar minHeight = *std::min_element(heightfieldData.begin(), heightfieldData.end());
        const btScalar maxHeight = *std::max_element(heightfieldData.begin(), heightfieldData.end());
        const int upAxis = 2;
        const PHY_ScalarType heightDataType = PHY_FLOAT;
        const bool flipQuadEdges = false;
        btHeightfieldTerrainShape shape(
            heightStickWidth,
            heightStickLength,
            heightfieldData.data(),
            heightScale,
            minHeight,
            maxHeight,
            upAxis,
            heightDataType,
            flipQuadEdges
        );
        btCollisionObject object;
        shape.setLocalScaling(btVector3(128, 128, 1));
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)));
        object.setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
        collisionWorld.addCollisionObject(&object);
        collisionWorld.updateAabbs();

        const btVector3 initial = onGround(btVector3(-2000, 0, maxAllowedHeight + 100));
        const btVector3 goal = onGround(btVector3(2000, 0, maxAllowedHeight + 100));

        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);

        for (const auto& object : objects)
        {
            collisionWorld.removeCollisionObject(object.get());
        }
        collisionWorld.removeCollisionObject(&object);
    }

}
