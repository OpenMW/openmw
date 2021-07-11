#include "operators.hpp"

#include <components/detournavigator/recastmeshbuilder.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/exceptions.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <DetourCommon.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>

namespace DetourNavigator
{
    static inline bool operator ==(const RecastMesh::Water& lhs, const RecastMesh::Water& rhs)
    {
        return lhs.mCellSize == rhs.mCellSize && lhs.mTransform == rhs.mTransform;
    }
}

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorRecastMeshBuilderTest : Test
    {
        Settings mSettings;
        TileBounds mBounds;
        const std::size_t mGeneration = 0;
        const std::size_t mRevision = 0;

        DetourNavigatorRecastMeshBuilderTest()
        {
            mSettings.mRecastScaleFactor = 1.0f;
            mBounds.mMin = osg::Vec2f(-std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon(),
                                      -std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon());
            mBounds.mMax = osg::Vec2f(std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon(),
                                      std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon());
        }
    };

    TEST_F(DetourNavigatorRecastMeshBuilderTest, create_for_empty_should_return_empty)
    {
        RecastMeshBuilder builder(mSettings, mBounds);
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>());
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>());
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>());
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);

        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -1, 0, -1,
            -1, 0, 1,
            1, 0, -1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            0, 3, 0,
            0, 3, 4,
            2, 3, 0,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_heightfield_terrian_shape)
    {
        const std::array<btScalar, 4> heightfieldData {{0, 0, 0, 0}};
        btHeightfieldTerrainShape shape(2, 2, heightfieldData.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -0.5, 0, -0.5,
            -0.5, 0, 0.5,
            0.5, 0, -0.5,
            0.5, 0, 0.5,
        }));
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({0, 1, 2, 2, 1, 3}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground, AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_box_shape_should_produce_12_triangles)
    {
        btBoxShape shape(btVector3(1, 1, 2));
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -1, -2, -1,
            -1, -2, 1,
            -1, 2, -1,
            -1, 2, 1,
            1, -2, -1,
            1, -2, 1,
            1, 2, -1,
            1, 2, 1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({
            0, 1, 3,
            0, 2, 6,
            0, 4, 5,
            1, 5, 7,
            2, 3, 7,
            3, 2, 0,
            4, 6, 7,
            5, 1, 0,
            6, 4, 0,
            7, 3, 1,
            7, 5, 4,
            7, 6, 2,
        })) << recastMesh->getMesh().getIndices();
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>(12, AreaType_ground));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_compound_shape)
    {
        btTriangleMesh mesh1;
        mesh1.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle1(&mesh1, true);
        btBoxShape box(btVector3(1, 1, 2));
        btTriangleMesh mesh2;
        mesh2.addTriangle(btVector3(1, 1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle2(&mesh2, true);
        btCompoundShape shape;
        shape.addChildShape(btTransform::getIdentity(), &triangle1);
        shape.addChildShape(btTransform::getIdentity(), &box);
        shape.addChildShape(btTransform::getIdentity(), &triangle2);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform::getIdentity(),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -1, -2, -1,
            -1, -2, 1,
            -1, 0, -1,
            -1, 0, 1,
            -1, 2, -1,
            -1, 2, 1,
            1, -2, -1,
            1, -2, 1,
            1, 0, -1,
            1, 0, 1,
            1, 2, -1,
            1, 2, 1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({
            0, 1, 5,
            0, 4, 10,
            0, 6, 7,
            1, 7, 11,
            4, 5, 11,
            5, 4, 0,
            6, 10, 11,
            7, 1, 0,
            8, 3, 2,
            8, 3, 9,
            10, 6, 0,
            11, 5, 1,
            11, 7, 6,
            11, 10, 4,
        })) << recastMesh->getMesh().getIndices();
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>(14, AreaType_ground));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_compound_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle(&mesh, true);
        btCompoundShape shape;
        shape.addChildShape(btTransform::getIdentity(), &triangle);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            0, 3, 0,
            0, 3, 4,
            2, 3, 0,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_compound_shape_with_transformed_bhv_triangle_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle(&mesh, true);
        btCompoundShape shape;
        shape.addChildShape(btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)),
                            &triangle);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            1, 12, 2,
            1, 12, 10,
            3, 12, 2,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, without_bounds_add_bhv_triangle_shape_should_not_filter_by_bounds)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        mesh.addTriangle(btVector3(-3, -3, 0), btVector3(-3, -2, 0), btVector3(-2, -3, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform::getIdentity(),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -3, 0, -3,
            -3, 0, -2,
            -2, 0, -3,
            -1, 0, -1,
            -1, 0, 1,
            1, 0, -1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0, 5, 4, 3}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>(2, AreaType_ground));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, with_bounds_add_bhv_triangle_shape_should_filter_by_bounds)
    {
        mSettings.mRecastScaleFactor = 0.1f;
        mBounds.mMin = osg::Vec2f(-3, -3) * mSettings.mRecastScaleFactor;
        mBounds.mMax = osg::Vec2f(-2, -2) * mSettings.mRecastScaleFactor;
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        mesh.addTriangle(btVector3(-3, -3, 0), btVector3(-3, -2, 0), btVector3(-2, -3, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform::getIdentity(),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -0.3f, 0, -0.3f,
            -0.3f, 0, -0.2f,
            -0.2f, 0, -0.3f,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, with_bounds_add_rotated_by_x_bhv_triangle_shape_should_filter_by_bounds)
    {
        mBounds.mMin = osg::Vec2f(-5, -5);
        mBounds.mMax = osg::Vec2f(5, -2);
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(0, -1, -1), btVector3(0, -1, -1), btVector3(0, 1, -1));
        mesh.addTriangle(btVector3(0, -3, -3), btVector3(0, -3, -2), btVector3(0, -2, -3));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btQuaternion(btVector3(1, 0, 0),
            static_cast<btScalar>(-osg::PI_4))),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_THAT(recastMesh->getMesh().getVertices(), Pointwise(FloatNear(1e-5), std::vector<float>({
            0, -0.707106769084930419921875, -3.535533905029296875,
            0, 4.44089209850062616169452667236328125e-16, -4.24264049530029296875,
            0, 0.707106769084930419921875, -3.535533905029296875,
        }))) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({0, 2, 1}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, with_bounds_add_rotated_by_y_bhv_triangle_shape_should_filter_by_bounds)
    {
        mBounds.mMin = osg::Vec2f(-5, -5);
        mBounds.mMax = osg::Vec2f(-3, 5);
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, 0, -1), btVector3(-1, 0, 1), btVector3(1, 0, -1));
        mesh.addTriangle(btVector3(-3, 0, -3), btVector3(-3, 0, -2), btVector3(-2, 0, -3));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btQuaternion(btVector3(0, 1, 0),
            static_cast<btScalar>(osg::PI_4))),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_THAT(recastMesh->getMesh().getVertices(), Pointwise(FloatNear(1e-5), std::vector<float>({
            -4.24264049530029296875, 4.44089209850062616169452667236328125e-16, 0,
            -3.535533905029296875, -0.707106769084930419921875, 0,
            -3.535533905029296875, 0.707106769084930419921875, 0,
        }))) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({1, 2, 0}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, with_bounds_add_rotated_by_z_bhv_triangle_shape_should_filter_by_bounds)
    {
        mBounds.mMin = osg::Vec2f(-5, -5);
        mBounds.mMax = osg::Vec2f(-1, -1);
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        mesh.addTriangle(btVector3(-3, -3, 0), btVector3(-3, -2, 0), btVector3(-2, -3, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape),
            btTransform(btQuaternion(btVector3(0, 0, 1),
            static_cast<btScalar>(osg::PI_4))),
            AreaType_ground
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_THAT(recastMesh->getMesh().getVertices(), Pointwise(FloatNear(1e-5), std::vector<float>({
            -1.41421353816986083984375, 0, -1.1102230246251565404236316680908203125e-16,
            1.1102230246251565404236316680908203125e-16, 0, -1.41421353816986083984375,
            1.41421353816986083984375, 0, 1.1102230246251565404236316680908203125e-16,
        }))) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 0, 1}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, flags_values_should_be_corresponding_to_added_objects)
    {
        btTriangleMesh mesh1;
        mesh1.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape1(&mesh1, true);
        btTriangleMesh mesh2;
        mesh2.addTriangle(btVector3(-3, -3, 0), btVector3(-3, -2, 0), btVector3(-2, -3, 0));
        btBvhTriangleMeshShape shape2(&mesh2, true);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(
            static_cast<const btCollisionShape&>(shape1),
            btTransform::getIdentity(),
            AreaType_ground
        );
        builder.addObject(
            static_cast<const btCollisionShape&>(shape2),
            btTransform::getIdentity(),
            AreaType_null
        );
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -3, 0, -3,
            -3, 0, -2,
            -2, 0, -3,
            -1, 0, -1,
            -1, 0, 1,
            1, 0, -1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0, 5, 4, 3}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_null, AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_water_then_get_water_should_return_it)
    {
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addWater(1000, btTransform(btMatrix3x3::getIdentity(), btVector3(100, 200, 300)));
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getWater(), std::vector<RecastMesh::Water>({
            RecastMesh::Water {1000, btTransform(btMatrix3x3::getIdentity(), btVector3(100, 200, 300))}
        }));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_bhv_triangle_mesh_shape_with_duplicated_vertices)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        mesh.addTriangle(btVector3(1, 1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);

        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = std::move(builder).create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getMesh().getVertices(), std::vector<float>({
            -1, 0, -1,
            -1, 0, 1,
            1, 0, -1,
            1, 0, 1,
        })) << recastMesh->getMesh().getVertices();
        EXPECT_EQ(recastMesh->getMesh().getIndices(), std::vector<int>({2, 1, 0, 2, 1, 3}));
        EXPECT_EQ(recastMesh->getMesh().getAreaTypes(), std::vector<AreaType>({AreaType_ground, AreaType_ground}));
    }
}
