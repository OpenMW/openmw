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

    TEST_F(DetourNavigatorRecastMeshBuilderTest, add_bhv_triangle_mesh_shape)
    {
        btTriangleMesh mesh;
        mesh.addTriangle(btVector3(-1, -1, 0), btVector3(-1, 1, 0), btVector3(1, -1, 0));
        btBvhTriangleMeshShape shape(&mesh, true);
        mBuilder.addObject(shape, btTransform::getIdentity());
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
        mBuilder.addObject(shape,
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
        mBuilder.addObject(shape, btTransform::getIdentity());
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
}
