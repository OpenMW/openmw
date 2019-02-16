#include "operators.hpp"

#include <components/detournavigator/tilecachedrecastmeshmanager.hpp>
#include <components/detournavigator/settingsutils.hpp>

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

    struct DetourNavigatorTileCachedRecastMeshManagerTest : Test
    {
        Settings mSettings;

        DetourNavigatorTileCachedRecastMeshManagerTest()
        {
            mSettings.mBorderSize = 16;
            mSettings.mCellSize = 0.2f;
            mSettings.mRecastScaleFactor = 0.017647058823529415f;
            mSettings.mTileSize = 64;
            mSettings.mTrianglesPerChunk = 256;
        }
    };

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_empty_should_return_nullptr)
    {
        TileCachedRecastMeshManager manager(mSettings);
        EXPECT_EQ(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, has_tile_for_empty_should_return_false)
    {
        TileCachedRecastMeshManager manager(mSettings);
        EXPECT_FALSE(manager.hasTile(TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_for_empty_should_return_zero)
    {
        const TileCachedRecastMeshManager manager(mSettings);
        EXPECT_EQ(manager.getRevision(), 0);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, for_each_tile_position_for_empty_should_call_none)
    {
        TileCachedRecastMeshManager manager(mSettings);
        std::size_t calls = 0;
        manager.forEachTilePosition([&] (const TilePosition&) { ++calls; });
        EXPECT_EQ(calls, 0);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_after_add_object_should_return_recast_mesh_for_each_used_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(1ul), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_after_add_object_should_return_nullptr_for_unused_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(1ul), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_EQ(manager.getMesh(TilePosition(1, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_moved_object_should_return_recast_mesh_for_each_used_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        const btTransform transform(btMatrix3x3::getIdentity(), btVector3(getTileSize(mSettings) / mSettings.mRecastScaleFactor, 0, 0));

        manager.addObject(ObjectId(1ul), boxShape, transform, AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(1, -1)), nullptr);

        manager.updateObject(ObjectId(1ul), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_moved_object_should_return_nullptr_for_unused_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        const btTransform transform(btMatrix3x3::getIdentity(), btVector3(getTileSize(mSettings) / mSettings.mRecastScaleFactor, 0, 0));

        manager.addObject(ObjectId(1ul), boxShape, transform, AreaType::AreaType_ground);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, 0)), nullptr);

        manager.updateObject(ObjectId(1ul), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_EQ(manager.getMesh(TilePosition(1, 0)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(1, -1)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_removed_object_should_return_nullptr_for_all_previously_used_tiles)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(1ul), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        manager.removeObject(ObjectId(1ul));
        EXPECT_EQ(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(0, 0)), nullptr);
    }
}
