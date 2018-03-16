#include "collisionobjects.hpp"

#include "apps/openmw/mwmechanics/findoptimalpath/get_neighbors.hpp"

#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>

#include <gtest/gtest.h>

inline std::ostream& operator <<(std::ostream& stream, const btVector3& value)
{
    return stream << std::setprecision(std::numeric_limits<btScalar>::digits)
        << "(" << value.x() << ", " << value.y() << ", " << value.z() << ")";
}

namespace
{

    using namespace testing;
    using namespace MWMechanics;

    using MWMechanics::FindOptimalPath::GetNeighbors;
    using MWPhysics::Collision;

    struct GetNeightborsTest : Test
    {
        btScalar minStep = std::numeric_limits<btScalar>::min();
        btScalar maxStep = std::numeric_limits<btScalar>::max();
        btScalar horizontalMargin = 1;
        btScalar verticalMargin = 1;
        bool allowFly = false;

        Sphere<400, 0, 125> sphere;
        Capsule capsule;
        Box<400, 0, 100> box;
        Floor floor;
        Stair stair;
        HeightField heightField;
    };

    TEST_F(GetNeightborsTest, with_sphere)
    {
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &sphere.object;
        collision.mFraction = 0.338400036096572875976562;
        collision.mNormal = btVector3(-0.99736011028289794921875, 0, -0.0726152881979942321777344);
        collision.mPoint = btVector3(150.660003662109375, 0, 106.8461761474609375);
        collision.mEnd = btVector3(121.45731353759765625, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(246.2955474853515625, 195.2046966552734375, 67.5), false},
            {btVector3(246.2955474853515625, -195.2046966552734375, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_sphere_when_allow_fly)
    {
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &sphere.object;
        collision.mFraction = 0.338400036096572875976562;
        collision.mNormal = btVector3(-0.99736011028289794921875, 0, -0.0726152881979942321777344);
        collision.mPoint = btVector3(150.660003662109375, 0, 106.8461761474609375);
        collision.mEnd = btVector3(121.45731353759765625, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly = true);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(246.2955474853515625, 195.2046966552734375, 67.5), false},
            {btVector3(246.2955474853515625, -195.2046966552734375, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_capsule)
    {
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &capsule.object;
        collision.mFraction = 0.4268000125885009765625;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(370.720001220703125, 0, 104.720001220703125);
        collision.mEnd = btVector3(341.44000244140625, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(397.783477783203125, 30.198760986328125, 67.5), false},
            {btVector3(397.783477783203125, -30.198760986328125, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_capsule_when_allow_fly)
    {
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &capsule.object;
        collision.mFraction = 0.4268000125885009765625;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(370.720001220703125, 0, 104.720001220703125);
        collision.mEnd = btVector3(341.44000244140625, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly = true);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(397.783477783203125, 30.198760986328125, 67.5), false},
            {btVector3(397.783477783203125, -30.198760986328125, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_by_x_when_allow_fly)
    {
        const btVector3 source {0, 0, 100};
        const btVector3 destination {800, 0, 100};
        const btVector3 goal {800, 0, 100};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.33840000629425048828125;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(300, 0, 100);
        collision.mEnd = btVector3(270.720001220703125, 0, 100);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly = true);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(269.720001220703125, 0, 200), false},
            {btVector3(269.720001220703125, 0, 0), false},
            {btVector3(269.720001220703125, 0, 100), true},
            {btVector3(269.720001220703125, 100, 100), false},
            {btVector3(269.720001220703125, -100, 100), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_by_y_when_allow_fly)
    {
        const btVector3 source {400, -400, 100};
        const btVector3 destination {400, 400, 100};
        const btVector3 goal {400, 400, 100};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.33840000629425048828125;
        collision.mNormal = btVector3(0, -1, 0);
        collision.mPoint = btVector3(400, -100, 100);
        collision.mEnd = btVector3(400, -129.279998779296875, 100);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly = true);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(400, -130.279998779296875, 200), false},
            {btVector3(400, -130.279998779296875, 0), false},
            {btVector3(400, -130.279998779296875, 100), true},
            {btVector3(300, -130.279998779296875, 100), false},
            {btVector3(500, -130.279998779296875, 100), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_by_z_when_allow_fly)
    {
        const btVector3 source {400, 0, -300};
        const btVector3 destination {400, 0, 500};
        const btVector3 goal {800, 0, 100};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.29187500476837158203125;
        collision.mNormal = btVector3(0, 0, -1);
        collision.mPoint = btVector3(400, 0, 0);
        collision.mEnd = btVector3(400, 0, -66.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly = true);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(300, 0, -67.5), false},
            {btVector3(500, 0, -67.5), false},
            {btVector3(400, 0, -67.5), true},
            {btVector3(400, -100, -67.5), false},
            {btVector3(400, 100, -67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_where_front_has_x_normal)
    {
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.338400036096572875976562;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(300, 0, 76.3179779052734375);
        collision.mEnd = btVector3(270.72003173828125, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(269.72003173828125, 0, 67.5), true},
            {btVector3(269.72003173828125, 100, 67.5), false},
            {btVector3(269.72003173828125, -100, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_where_front_has_y_normal)
    {
        box.object.setWorldTransform(btTransform(btQuaternion(btVector3(0, 0, 1), btScalar(osg::PI_2)), box.origin));
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.338400036096572875976562;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(300, 0, 76.3179779052734375);
        collision.mEnd = btVector3(270.72003173828125, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(269.72003173828125, 0, 67.5), true},
            {btVector3(269.72003173828125, 100, 67.5), false},
            {btVector3(269.72003173828125, -100, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_box_where_front_has_z_normal)
    {
        box.object.setWorldTransform(btTransform(btQuaternion(btVector3(0, 1, 0), btScalar(osg::PI_2)), box.origin));
        const btVector3 source {0, 0, 67.5};
        const btVector3 destination {800, 0, 67.5};
        const btVector3 goal {800, 0, 67.5};
        Collision collision;
        collision.mObject = &box.object;
        collision.mFraction = 0.338400036096572875976562;
        collision.mNormal = btVector3(-1, 0, 0);
        collision.mPoint = btVector3(300, 0, 76.3179779052734375);
        collision.mEnd = btVector3(270.72003173828125, 0, 67.5);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(269.72003173828125, 0, 67.5), true},
            {btVector3(269.72003173828125, 100, 67.5), false},
            {btVector3(269.72003173828125, -100, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_plane)
    {
        const btVector3 source {0, 0, 66.51000213623046875};
        const btVector3 destination {365.31549072265625, 0, 54.498546600341796875};
        const btVector3 goal {800, 0, 66.51000213623046875};
        Collision collision;
        collision.mObject = &floor.object;
        collision.mFraction = 0.000832716410513967275619507;
        collision.mNormal = btVector3(0, 0, 1);
        collision.mPoint = btVector3(0.304204195737838745117188, 0, 0);
        collision.mEnd = btVector3(0.304204195737838745117188, 0, 66.5);

        minStep = 1;
        maxStep = 800;

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(1.3042042255401611328125, 0, 67.5), false},
            {btVector3(-0.6957957744598388671875, 0, 67.5), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_stair)
    {
        const btVector3 source {0, 0, 66.51000213623046875};
        const btVector3 destination {365.31549072265625, 0, 54.498546600341796875};
        const btVector3 goal {800, 0, 66.51000213623046875};
        Collision collision;
        collision.mObject = &stair.object;
        collision.mFraction = 0.473426431417465209960938;
        collision.mNormal = btVector3(-0.707106769084930419921875, 0, 0.7071068286895751953125);
        collision.mPoint = btVector3(399.44451904296875, 0, 8.586639404296875);
        collision.mEnd = btVector3(378.74114990234375, 0, 66.51000213623046875);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(385.105133056640625, 0, 74.288177490234375), false},
            {btVector3(370.962982177734375, 0, 60.1460418701171875), false},
        });
        EXPECT_EQ(result, expected);
    }

    TEST_F(GetNeightborsTest, with_height_field)
    {
        const btVector3 source {0, 0, 1};
        const btVector3 destination {10, 0, 1};
        const btVector3 goal {1, 0, 1};
        Collision collision;
        collision.mObject = &heightField.object;
        collision.mFraction = 0;
        collision.mNormal = btVector3(0, 0, 1);
        collision.mPoint = btVector3(0, 0, 0);
        collision.mEnd = btVector3(0, 0, 1);

        const GetNeighbors getNeighbors(source, destination, goal, collision, minStep, maxStep, horizontalMargin, verticalMargin, allowFly);
        const auto result = getNeighbors.perform();
        const std::vector<std::pair<btVector3, bool>> expected({
            {btVector3(2, 0, 2), false},
            {btVector3(-2, 0, 2), false},
        });
        EXPECT_EQ(result, expected);
    }

}
