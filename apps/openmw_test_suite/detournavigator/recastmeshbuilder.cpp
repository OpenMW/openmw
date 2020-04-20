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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
            mSettings.mTrianglesPerChunk = 256;
            mBounds.mMin = osg::Vec2f(-std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon(),
                                      -std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon());
            mBounds.mMax = osg::Vec2f(std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon(),
                                      std::numeric_limits<float>::max() * std::numeric_limits<float>::epsilon());
        }
    };

    TEST_F(DetourNavigatorRecastMeshBuilderTest, create_for_empty_should_return_empty)
    {
        RecastMeshBuilder builder(mSettings, mBounds);
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>());
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>());
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>());
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);

        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 0, -1,
            -1, 0, 1,
            -1, 0, -1,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            2, 3, 0,
            0, 3, 4,
            0, 3, 0,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_heightfield_terrian_shape)
    {
        const std::array<btScalar, 4> heightfieldData {{0, 0, 0, 0}};
        btHeightfieldTerrainShape shape(2, 2, heightfieldData.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            -0.5, 0, -0.5,
            -0.5, 0, 0.5,
            0.5, 0, -0.5,
            0.5, 0, -0.5,
            -0.5, 0, 0.5,
            0.5, 0, 0.5,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2, 3, 4, 5}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground, AreaType_ground}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_box_shape_should_produce_12_triangles)
    {
        btBoxShape shape(btVector3(1, 1, 2));
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity(), AreaType_ground);
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 2, 1,
            -1, 2, 1,
            1, 2, -1,
            -1, 2, -1,
            1, -2, 1,
            -1, -2, 1,
            1, -2, -1,
            -1, -2, -1,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({
            0, 2, 3,
            3, 1, 0,
            0, 4, 6,
            6, 2, 0,
            0, 1, 5,
            5, 4, 0,
            7, 5, 1,
            1, 3, 7,
            7, 3, 2,
            2, 6, 7,
            7, 6, 4,
            4, 5, 7,
        }));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>(12, AreaType_ground));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 0, -1,
            -1, 0, 1,
            -1, 0, -1,
            1, 2, 1,
            -1, 2, 1,
            1, 2, -1,
            -1, 2, -1,
            1, -2, 1,
            -1, -2, 1,
            1, -2, -1,
            -1, -2, -1,
            1, 0, -1,
            -1, 0, 1,
            1, 0, 1,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({
            0, 1, 2,
            3, 5, 6,
            6, 4, 3,
            3, 7, 9,
            9, 5, 3,
            3, 4, 8,
            8, 7, 3,
            10, 8, 4,
            4, 6, 10,
            10, 6, 5,
            5, 9, 10,
            10, 9, 7,
            7, 8, 10,
            11, 12, 13,
        }));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>(14, AreaType_ground));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            2, 3, 0,
            0, 3, 4,
            0, 3, 0,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            3, 12, 2,
            1, 12, 10,
            1, 12, 2,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 0, -1,
            -1, 0, 1,
            -1, 0, -1,
            -2, 0, -3,
            -3, 0, -2,
            -3, 0, -3,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2, 3, 4, 5}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>(2, AreaType_ground));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            -0.2f, 0, -0.3f,
            -0.3f, 0, -0.2f,
            -0.3f, 0, -0.3f,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_THAT(recastMesh->getVertices(), Pointwise(FloatNear(1e-5), std::vector<float>({
            0, -0.70710659027099609375, -3.535533905029296875,
            0, 0.707107067108154296875, -3.535533905029296875,
            0, 2.384185791015625e-07, -4.24264049530029296875,
        })));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_THAT(recastMesh->getVertices(), Pointwise(FloatNear(1e-5), std::vector<float>({
            -3.535533905029296875, -0.70710659027099609375, 0,
            -3.535533905029296875, 0.707107067108154296875, 0,
            -4.24264049530029296875, 2.384185791015625e-07, 0,
        })));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1.41421353816986083984375, 0, 1.1920928955078125e-07,
            -1.41421353816986083984375, 0, -1.1920928955078125e-07,
            1.1920928955078125e-07, 0, -1.41421353816986083984375,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground}));
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
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 0, -1,
            -1, 0, 1,
            -1, 0, -1,
            -2, 0, -3,
            -3, 0, -2,
            -3, 0, -3,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2, 3, 4, 5}));
        EXPECT_EQ(recastMesh->getAreaTypes(), std::vector<AreaType>({AreaType_ground, AreaType_null}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_water_then_get_water_should_return_it)
    {
        RecastMeshBuilder builder(mSettings, mBounds);
        builder.addWater(1000, btTransform(btMatrix3x3::getIdentity(), btVector3(100, 200, 300)));
        const auto recastMesh = builder.create(mGeneration, mRevision);
        EXPECT_EQ(recastMesh->getWater(), std::vector<RecastMesh::Water>({
            RecastMesh::Water {1000, btTransform(btMatrix3x3::getIdentity(), btVector3(100, 200, 300))}
        }));
    }
}
