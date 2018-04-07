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

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorRecastMeshBuilderTest : Test
    {
        Settings mSettings;
        RecastMeshBuilder mBuilder;

        DetourNavigatorRecastMeshBuilderTest()
            : mBuilder(mSettings)
        {
            mSettings.mRecastScaleFactor = 1.0f;
            mSettings.mTrianglesPerChunk = 256;
        }
    };

    TEST_F(DetourNavigatorRecastMeshBuilderTest, create_for_empty_should_throw_exception)
    {
        EXPECT_THROW(mBuilder.create(), std::invalid_argument);
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity());
        const auto recastMesh = mBuilder.create();
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            1, 0, -1,
            -1, 0, 1,
            -1, 0, -1,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape),
                           btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)));
        const auto recastMesh = mBuilder.create();
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            2, 3, 0,
            0, 3, 4,
            0, 3, 0,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_heightfield_terrian_shape)
    {
        const std::array<btScalar, 4> heightfieldData {{0, 0, 0, 0}};
        btHeightfieldTerrainShape shape(2, 2, heightfieldData.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity());
        const auto recastMesh = mBuilder.create();
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            -0.5, 0, -0.5,
            -0.5, 0, 0.5,
            0.5, 0, -0.5,
            0.5, 0, -0.5,
            -0.5, 0, 0.5,
            0.5, 0, 0.5,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2, 3, 4, 5}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_box_shape_should_produce_12_triangles)
    {
        btBoxShape shape(btVector3(1, 1, 2));
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity());
        const auto recastMesh = mBuilder.create();
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
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape), btTransform::getIdentity());
        const auto recastMesh = mBuilder.create();
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
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_compound_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle(&mesh, true);
        btCompoundShape shape;
        shape.addChildShape(btTransform::getIdentity(), &triangle);
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape),
                           btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)));
        const auto recastMesh = mBuilder.create();
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            2, 3, 0,
            0, 3, 4,
            0, 3, 0,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
    }

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_transformed_compound_shape_with_transformed_bhv_triangle_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape triangle(&mesh, true);
        btCompoundShape shape;
        shape.addChildShape(btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)),
                            &triangle);
        mBuilder.addObject(static_cast<const btCollisionShape&>(shape),
                           btTransform(btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 3)), btVector3(1, 2, 3)));
        const auto recastMesh = mBuilder.create();
        EXPECT_EQ(recastMesh->getVertices(), std::vector<float>({
            3, 12, 2,
            1, 12, 10,
            1, 12, 2,
        }));
        EXPECT_EQ(recastMesh->getIndices(), std::vector<int>({0, 1, 2}));
    }
}
