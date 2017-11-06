#include "collisionobjects.hpp"
#include "matchers.hpp"

#include "apps/openmw/mwmechanics/findoptimalpath.hpp"

#include "apps/openmw/mwphysics/closestcollision.hpp"

#include <osg/Math>

#include <boost/optional.hpp>

#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
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

    struct FindOptimalPathScenariosTest : Test
    {
        Sphere<400, 0, 125> sphere;
        Sphere<100, 300, 125> sphere2;
        Sphere<100, -300, 125> sphere3;
        Capsule capsule;
        Box<400, 0, 100> box;
        Box<200, 200, 100> box2;
        Box<200, -200, 100> box3;
        Compound compound;
        Floor floor;
        Stair stair;
        LeftWall leftWall;
        RightWall rightWall;
        Roof roof;
        Sphere<44, -144, 67, 20> sphereAtLeftTangent;
        Sphere<44, 144, 67, 20> sphereAtRightTangent;
        FirstPlatform firstPlatform;
        SecondLoweredPlatform secondLoweredPlatform;
        ThirdPlatform thirdPlatform;
        Slope slope;
        ElevatedPlatform elevatedPlatform;
        SteepSlope steepSlope;
        UnreachableElevatedPlatform unreachableElevatedPlatform;
        EllispoidY ellispoidY;
        TurnBackWall turnBackWall;
        TurnFrontWall turnFrontWall;
        TurnLeftWall turnLeftWall;
        TurnRightWall turnRightWall;
        TurnInside turnInside;
        UTurnInsideWall uTurnInsideWall;
        UTurnRightWall uTurnRightWall;
        Mesh mesh;
        Mesh2 mesh2;
        HeightField heightField;
        HeightField2 heightField2;
        Player player;
        Mesh3 mesh3;
        Mesh4 mesh4;
        Player2 player2;
        Actor actor;
        btDefaultCollisionConfiguration collisionConfiguration;
        btCollisionDispatcher dispatcher {&collisionConfiguration};
        btDbvtBroadphase broadphase;
        btCollisionWorld collisionWorld {&dispatcher, &broadphase, &collisionConfiguration};
        FindOptimalPathConfig config;

        FindOptimalPathScenariosTest()
        {
            config.mAllowFly = false;
            config.mActorHalfExtents = actor.halfExtents;
            config.mMaxDepth = std::numeric_limits<std::size_t>::max();
            config.mMaxIterations = std::numeric_limits<std::size_t>::max();
            config.mHasNearCollisionFilterFactor = 0.33f;
            config.mHasNearCollisionFilterReduceFactor = 0.5f;
            config.mHorizontalMarginFactor = 2;

            collisionWorld.addCollisionObject(&actor.object);
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
//                ASSERT_THAT(Transition(initial, point), MedianPointIsOnGround(this));
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

    inline btScalar length(btVector3 initial, const std::vector<btVector3>& points)
    {
        return std::accumulate(points.begin(), points.end(), btScalar(0),
            [&] (btScalar s, const btVector3& v) {
                const auto r = s + initial.distance(v);
                initial = v;
                return r;
            });
    }

    TEST_F(FindOptimalPathScenariosTest, without_objects)
    {
        config.mAllowFly = true;
        config.mMaxDepth = 1;
        config.mMaxIterations = 1;
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 800, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor)
    {
        config.mMaxDepth = 1;
        config.mMaxIterations = 1;
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 800, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_left_and_right_walls)
    {
        config.mMaxDepth = 1;
        config.mMaxIterations = 1;
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&leftWall.object);
        collisionWorld.addCollisionObject(&rightWall.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 800, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_roof)
    {
        config.mMaxDepth = 1;
        config.mMaxIterations = 1;
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&roof.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 800, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_roof_and_left_and_right_walls)
    {
        config.mMaxDepth = 1;
        config.mMaxIterations = 1;
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&roof.object);
        collisionWorld.addCollisionObject(&leftWall.object);
        collisionWorld.addCollisionObject(&rightWall.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 800, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_sphere_on_the_way)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&sphere.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1045, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_sphere_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&sphere.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1045, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_three_spheres_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&sphere.object);
        collisionWorld.addCollisionObject(&sphere2.object);
        collisionWorld.addCollisionObject(&sphere3.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1805, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_sphere_on_the_way_and_spheres_at_tangents_to_first)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&sphere.object);
        collisionWorld.addCollisionObject(&sphereAtLeftTangent.object);
        collisionWorld.addCollisionObject(&sphereAtRightTangent.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin - btVector3(200, 0, 0);
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1228, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_capsule_on_the_way)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&capsule.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 821, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_capsule_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&capsule.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 821, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_box_by_x_on_the_way)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = box.origin - btVector3(400, 0, 0);
        const btVector3 goal = box.origin + btVector3(400, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 959, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_box_by_y_on_the_way)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = box.origin - btVector3(0, 400, 0);
        const btVector3 goal = box.origin + btVector3(0, 400, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 959, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_box_by_z_on_the_way)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = box.origin - btVector3(0, 0, 400);
        const btVector3 goal = box.origin + btVector3(0, 0, 400);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 974, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_box_where_front_has_x_normal_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 959, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_box_where_front_has_y_normal_on_the_way)
    {
        box.object.setWorldTransform(btTransform(btQuaternion(btVector3(0, 0, 1), btScalar(osg::PI_2)), box.origin));
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 959, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_box_where_front_has_z_normal_on_the_way)
    {
        box.object.setWorldTransform(btTransform(btQuaternion(btVector3(0, 1, 0), btScalar(osg::PI_2)), box.origin));
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = actor.origin;
        const btVector3 goal = actor.origin + btVector3(800, 0, 0);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 959, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_three_boxes_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&box.object);
        collisionWorld.addCollisionObject(&box2.object);
        collisionWorld.addCollisionObject(&box3.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1220, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_stair_on_the_floor)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&stair.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 802, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_compound_of_three_box_collision_objects_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&compound.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 865, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_sequence_of_three_platforms_where_second_is_lowered)
    {
        collisionWorld.addCollisionObject(&firstPlatform.object);
        collisionWorld.addCollisionObject(&secondLoweredPlatform.object);
        collisionWorld.addCollisionObject(&thirdPlatform.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(400, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsFarFrom(goal, 197));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_slope_to_the_elevated_platform)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&slope.object);
        collisionWorld.addCollisionObject(&elevatedPlatform.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const auto z = elevatedPlatform.origin.z() + elevatedPlatform.shape.getHalfExtentsWithMargin().z();
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, z));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 928, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_steep_slope_to_the_unreachable_elevated_platform)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&steepSlope.object);
        collisionWorld.addCollisionObject(&unreachableElevatedPlatform.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const auto z = unreachableElevatedPlatform.origin.z()
                + unreachableElevatedPlatform.shape.getHalfExtentsWithMargin().z();
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, z));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_TRUE(result.mPoints.empty());
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_ellipsoid_on_the_way)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&ellispoidY.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(actor.origin + btVector3(800, 0, 0));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 831, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_floor_and_capsule_goal_occupier)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&capsule.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(actor.origin);
        const btVector3 goal = onGround(capsule.origin);
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, capsule.halfExtents.x() + actor.halfExtents.x() + 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 341, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_turn)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&roof.object);
        collisionWorld.addCollisionObject(&turnBackWall.object);
        collisionWorld.addCollisionObject(&turnFrontWall.object);
        collisionWorld.addCollisionObject(&turnLeftWall.object);
        collisionWorld.addCollisionObject(&turnRightWall.object);
        collisionWorld.addCollisionObject(&turnInside.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(btVector3(-30, -970, actor.origin.z()));
        const btVector3 goal = onGround(btVector3(-970, -30, actor.origin.z()));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, capsule.halfExtents.x() + actor.halfExtents.x() + 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1880, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, DISABLED_with_uturn)
    {
        collisionWorld.addCollisionObject(&floor.object);
        collisionWorld.addCollisionObject(&roof.object);
        collisionWorld.addCollisionObject(&turnBackWall.object);
        collisionWorld.addCollisionObject(&turnFrontWall.object);
        collisionWorld.addCollisionObject(&turnLeftWall.object);
        collisionWorld.addCollisionObject(&uTurnRightWall.object);
        collisionWorld.addCollisionObject(&uTurnInsideWall.object);
        collisionWorld.updateAabbs();
        const btVector3 initial = onGround(btVector3(-970, -30, actor.origin.z()));
        const btVector3 goal = onGround(btVector3(-970, -91, actor.origin.z()));
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, capsule.halfExtents.x() + actor.halfExtents.x() + 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1801, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_mesh)
    {
        config.mAllowFly = true;
        collisionWorld.addCollisionObject(&mesh.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {56210.8125, -49236.421875, 549.3701171875};
        const btVector3 goal {56607, -49244, 726.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 619, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_height_field)
    {
        collisionWorld.addCollisionObject(&heightField.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {56210.8125, -49236.421875, 549.3701171875};
        const btVector3 goal {56607, -49244, 726.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 441, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_mesh_and_height_fields)
    {
        collisionWorld.addCollisionObject(&heightField.object);
        collisionWorld.addCollisionObject(&heightField2.object);
        collisionWorld.addCollisionObject(&mesh.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {56210.8125, -49236.421875, 549.3701171875};
        const btVector3 goal {56607, -49244, 726.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 938, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_meshes_and_height_fields)
    {
        collisionWorld.addCollisionObject(&heightField.object);
        collisionWorld.addCollisionObject(&heightField2.object);
        collisionWorld.addCollisionObject(&mesh.object);
        collisionWorld.addCollisionObject(&mesh2.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {56210.8125, -49236.421875, 549.3701171875};
        const btVector3 goal {56607, -49244, 726.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 1));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 938, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_meshes_and_height_fields_and_player)
    {
        collisionWorld.addCollisionObject(&heightField.object);
        collisionWorld.addCollisionObject(&heightField2.object);
        collisionWorld.addCollisionObject(&mesh.object);
        collisionWorld.addCollisionObject(&mesh2.object);
        collisionWorld.addCollisionObject(&player.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {56210.8125, -49236.421875, 549.3701171875};
        const btVector3 goal {56607, -49244, 726.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 62));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 878, 0.5);
    }

    TEST_F(FindOptimalPathScenariosTest, with_meshes_and_height_fields_and_player_2)
    {
        collisionWorld.addCollisionObject(&heightField2.object);
        collisionWorld.addCollisionObject(&mesh3.object);
        collisionWorld.addCollisionObject(&mesh4.object);
        collisionWorld.addCollisionObject(&player2.object);
        collisionWorld.updateAabbs();
        const btVector3 initial {55736.3125, -47998.01953125, 492.180938720703125};
        const btVector3 goal {55193, -48007, 635.5};
        const auto result = findOptimalPath(collisionWorld, actor.object, initial, goal, config);
        ASSERT_FALSE(result.mReachMaxIterations);
        ASSERT_FALSE(result.mPoints.empty());
        ASSERT_THAT(result.mPoints.back(), IsNearTo(goal, 62));
        assertThatAllPathIsOnGround(initial, result.mPoints);
//        assertThatNoCollisionsOnThePath(initial, result.mPoints);
        EXPECT_NEAR(length(initial, result.mPoints), 1046, 0.5);
    }

}
