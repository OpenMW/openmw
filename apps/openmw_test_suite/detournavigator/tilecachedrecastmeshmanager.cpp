#include "operators.hpp"

#include <components/detournavigator/tilecachedrecastmeshmanager.hpp>
#include <components/detournavigator/settingsutils.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorTileCachedRecastMeshManagerTest : Test
    {
        Settings mSettings;
        std::vector<TilePosition> mChangedTiles;

        DetourNavigatorTileCachedRecastMeshManagerTest()
        {
            mSettings.mBorderSize = 16;
            mSettings.mCellSize = 0.2f;
            mSettings.mRecastScaleFactor = 0.017647058823529415f;
            mSettings.mTileSize = 64;
            mSettings.mTrianglesPerChunk = 256;
        }

        void onChangedTile(const TilePosition& tilePosition)
        {
            mChangedTiles.push_back(tilePosition);
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

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, add_object_for_new_object_should_return_true)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        EXPECT_TRUE(manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground));
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, add_object_for_existing_object_should_return_false)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_FALSE(manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground));
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, update_object_for_changed_object_should_return_changed_tiles)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        const btTransform transform(btMatrix3x3::getIdentity(), btVector3(getTileSize(mSettings) / mSettings.mRecastScaleFactor, 0, 0));
        manager.addObject(ObjectId(&boxShape), boxShape, transform, AreaType::AreaType_ground);
        EXPECT_TRUE(manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground,
                                         [&] (const auto& v) { onChangedTile(v); }));
        EXPECT_THAT(
            mChangedTiles,
            ElementsAre(TilePosition(-1, -1), TilePosition(-1, 0), TilePosition(0, -1), TilePosition(0, 0),
                        TilePosition(1, -1), TilePosition(1, 0))
        );
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, update_object_for_not_changed_object_should_return_empty)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_FALSE(manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground,
                                          [&] (const auto& v) { onChangedTile(v); }));
        EXPECT_EQ(mChangedTiles, std::vector<TilePosition>());
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_after_add_object_should_return_recast_mesh_for_each_used_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_after_add_object_should_return_nullptr_for_unused_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_EQ(manager.getMesh(TilePosition(1, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_moved_object_should_return_recast_mesh_for_each_used_tile)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        const btTransform transform(btMatrix3x3::getIdentity(), btVector3(getTileSize(mSettings) / mSettings.mRecastScaleFactor, 0, 0));

        manager.addObject(ObjectId(&boxShape), boxShape, transform, AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(1, -1)), nullptr);

        manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground, [] (auto) {});
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

        manager.addObject(ObjectId(&boxShape), boxShape, transform, AreaType::AreaType_ground);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, 0)), nullptr);

        manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground, [] (auto) {});
        EXPECT_EQ(manager.getMesh(TilePosition(1, 0)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(1, -1)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_removed_object_should_return_nullptr_for_all_previously_used_tiles)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        manager.removeObject(ObjectId(&boxShape));
        EXPECT_EQ(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_EQ(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_mesh_for_not_changed_object_after_update_should_return_recast_mesh_for_same_tiles)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));

        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_NE(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);

        manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground, [] (auto) {});
        EXPECT_NE(manager.getMesh(TilePosition(-1, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(-1, 0)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, -1)), nullptr);
        EXPECT_NE(manager.getMesh(TilePosition(0, 0)), nullptr);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_add_object_new_should_return_incremented_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const auto initialRevision = manager.getRevision();
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_EQ(manager.getRevision(), initialRevision + 1);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_add_object_existing_should_return_same_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        const auto beforeAddRevision = manager.getRevision();
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        EXPECT_EQ(manager.getRevision(), beforeAddRevision);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_update_moved_object_should_return_incremented_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        const btTransform transform(btMatrix3x3::getIdentity(), btVector3(getTileSize(mSettings) / mSettings.mRecastScaleFactor, 0, 0));
        manager.addObject(ObjectId(&boxShape), boxShape, transform, AreaType::AreaType_ground);
        const auto beforeUpdateRevision = manager.getRevision();
        manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground, [] (auto) {});
        EXPECT_EQ(manager.getRevision(), beforeUpdateRevision + 1);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_update_not_changed_object_should_return_same_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        const auto beforeUpdateRevision = manager.getRevision();
        manager.updateObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground, [] (auto) {});
        EXPECT_EQ(manager.getRevision(), beforeUpdateRevision);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_remove_existing_object_should_return_incremented_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const btBoxShape boxShape(btVector3(20, 20, 100));
        manager.addObject(ObjectId(&boxShape), boxShape, btTransform::getIdentity(), AreaType::AreaType_ground);
        const auto beforeRemoveRevision = manager.getRevision();
        manager.removeObject(ObjectId(&boxShape));
        EXPECT_EQ(manager.getRevision(), beforeRemoveRevision + 1);
    }

    TEST_F(DetourNavigatorTileCachedRecastMeshManagerTest, get_revision_after_remove_absent_object_should_return_same_value)
    {
        TileCachedRecastMeshManager manager(mSettings);
        const auto beforeRemoveRevision = manager.getRevision();
        manager.removeObject(ObjectId(&manager));
        EXPECT_EQ(manager.getRevision(), beforeRemoveRevision);
    }
}
