#include "settings.hpp"

#include <components/detournavigator/asyncnavmeshupdater.hpp>
#include <components/detournavigator/dbrefgeometryobject.hpp>
#include <components/detournavigator/makenavmesh.hpp>
#include <components/detournavigator/navmeshdbutils.hpp>
#include <components/detournavigator/serialization.hpp>
#include <components/files/conversion.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/testing/util.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>

#include <DetourNavMesh.h>

#include <gtest/gtest.h>

#include <limits>
#include <map>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;
    using namespace DetourNavigator::Tests;

    void addHeightFieldPlane(
        TileCachedRecastMeshManager& recastMeshManager, const osg::Vec2i cellPosition = osg::Vec2i(0, 0))
    {
        const int cellSize = 8192;
        recastMeshManager.addHeightfield(cellPosition, cellSize, HeightfieldPlane{ 0 }, nullptr);
    }

    void addObject(const btBoxShape& shape, TileCachedRecastMeshManager& recastMeshManager)
    {
        const ObjectId id(&shape);
        osg::ref_ptr<Resource::BulletShape> bulletShape(new Resource::BulletShape);
        constexpr VFS::Path::NormalizedView test("test.nif");
        bulletShape->mFileName = test;
        bulletShape->mFileHash = "test_hash";
        ObjectTransform objectTransform;
        std::fill(std::begin(objectTransform.mPosition.pos), std::end(objectTransform.mPosition.pos), 0.1f);
        std::fill(std::begin(objectTransform.mPosition.rot), std::end(objectTransform.mPosition.rot), 0.2f);
        objectTransform.mScale = 3.14f;
        const CollisionShape collisionShape(
            osg::ref_ptr<Resource::BulletShapeInstance>(new Resource::BulletShapeInstance(bulletShape)), shape,
            objectTransform);
        recastMeshManager.addObject(id, collisionShape, btTransform::getIdentity(), AreaType_ground, nullptr);
    }

    struct DetourNavigatorAsyncNavMeshUpdaterTest : Test
    {
        Settings mSettings = makeSettings();
        TileCachedRecastMeshManager mRecastMeshManager{ mSettings.mRecast };
        OffMeshConnectionsManager mOffMeshConnectionsManager{ mSettings.mRecast };
        const AgentBounds mAgentBounds{ CollisionShapeType::Aabb, { 29, 29, 66 } };
        const TilePosition mPlayerTile{ 0, 0 };
        const ESM::RefId mWorldspace = ESM::RefId::stringRefId("sys::default");
        const btBoxShape mBox{ btVector3(100, 100, 20) };
        Loading::Listener mListener;
    };

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, for_all_jobs_done_when_empty_wait_should_terminate)
    {
        AsyncNavMeshUpdater updater{ mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr };
        updater.wait(WaitConditionType::allJobsDone, &mListener);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, for_required_tiles_present_when_empty_wait_should_terminate)
    {
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        updater.wait(WaitConditionType::requiredTilesPresent, &mListener);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_should_generate_navmesh_tile)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_NE(navMeshCacheItem->lockConst()->getImpl().getTileRefAt(0, 0, 0), 0u);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, repeated_post_should_lead_to_cache_hit)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            ASSERT_EQ(stats.mCache.mGetCount, 1);
            ASSERT_EQ(stats.mCache.mHitCount, 0);
        }
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            EXPECT_EQ(stats.mCache.mGetCount, 2);
            EXPECT_EQ(stats.mCache.mHitCount, 1);
        }
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_for_update_change_type_should_not_update_cache)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::update } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            ASSERT_EQ(stats.mCache.mGetCount, 1);
            ASSERT_EQ(stats.mCache.mHitCount, 0);
        }
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            EXPECT_EQ(stats.mCache.mGetCount, 2);
            EXPECT_EQ(stats.mCache.mHitCount, 0);
        }
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_should_write_generated_tile_to_db)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        addObject(mBox, mRecastMeshManager);
        auto db = std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max());
        NavMeshDb* const dbPtr = db.get();
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db));
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const TilePosition tilePosition{ 0, 0 };
        const std::map<TilePosition, ChangeType> changedTiles{ { tilePosition, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        updater.stop();
        const auto recastMesh = mRecastMeshManager.getMesh(mWorldspace, tilePosition);
        ASSERT_NE(recastMesh, nullptr);
        ShapeId nextShapeId{ 1 };
        const std::vector<DbRefGeometryObject> objects = makeDbRefGeometryObjects(recastMesh->getMeshSources(),
            [&](const MeshSource& v) { return resolveMeshSource(*dbPtr, v, nextShapeId); });
        const auto tile = dbPtr->findTile(
            mWorldspace, tilePosition, serialize(mSettings.mRecast, mAgentBounds, *recastMesh, objects));
        ASSERT_TRUE(tile.has_value());
        EXPECT_EQ(tile->mTileId, 1);
        EXPECT_EQ(tile->mVersion, navMeshFormatVersion);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_when_writing_to_db_disabled_should_not_write_tiles)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        addObject(mBox, mRecastMeshManager);
        auto db = std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max());
        NavMeshDb* const dbPtr = db.get();
        mSettings.mWriteToNavMeshDb = false;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db));
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const TilePosition tilePosition{ 0, 0 };
        const std::map<TilePosition, ChangeType> changedTiles{ { tilePosition, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        updater.stop();
        const auto recastMesh = mRecastMeshManager.getMesh(mWorldspace, tilePosition);
        ASSERT_NE(recastMesh, nullptr);
        ShapeId nextShapeId{ 1 };
        const std::vector<DbRefGeometryObject> objects = makeDbRefGeometryObjects(recastMesh->getMeshSources(),
            [&](const MeshSource& v) { return resolveMeshSource(*dbPtr, v, nextShapeId); });
        const auto tile = dbPtr->findTile(
            mWorldspace, tilePosition, serialize(mSettings.mRecast, mAgentBounds, *recastMesh, objects));
        ASSERT_FALSE(tile.has_value());
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_when_writing_to_db_disabled_should_not_write_shapes)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        addObject(mBox, mRecastMeshManager);
        auto db = std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max());
        NavMeshDb* const dbPtr = db.get();
        mSettings.mWriteToNavMeshDb = false;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db));
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const TilePosition tilePosition{ 0, 0 };
        const std::map<TilePosition, ChangeType> changedTiles{ { tilePosition, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        updater.stop();
        const auto recastMesh = mRecastMeshManager.getMesh(mWorldspace, tilePosition);
        ASSERT_NE(recastMesh, nullptr);
        const auto objects = makeDbRefGeometryObjects(
            recastMesh->getMeshSources(), [&](const MeshSource& v) { return resolveMeshSource(*dbPtr, v); });
        EXPECT_TRUE(std::holds_alternative<MeshSource>(objects));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, post_should_read_from_db_on_cache_miss)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mMaxNavMeshTilesCacheSize = 0;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager,
            std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max()));
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            ASSERT_EQ(stats.mCache.mGetCount, 1);
            ASSERT_EQ(stats.mCache.mHitCount, 0);
            ASSERT_TRUE(stats.mDb.has_value());
            ASSERT_EQ(stats.mDb->mGetTileCount, 1);
            ASSERT_EQ(stats.mDbGetTileHits, 0);
        }
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        {
            const auto stats = updater.getStats();
            EXPECT_EQ(stats.mCache.mGetCount, 2);
            EXPECT_EQ(stats.mCache.mHitCount, 0);
            ASSERT_TRUE(stats.mDb.has_value());
            EXPECT_EQ(stats.mDb->mGetTileCount, 2);
            EXPECT_EQ(stats.mDbGetTileHits, 1);
        }
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, on_changing_player_tile_post_should_remove_tiles_out_of_range)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTilesAdd{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTilesAdd);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        ASSERT_NE(navMeshCacheItem->lockConst()->getImpl().getTileRefAt(0, 0, 0), 0u);
        const std::map<TilePosition, ChangeType> changedTilesRemove{ { TilePosition{ 0, 0 }, ChangeType::remove } };
        const TilePosition playerTile(100, 100);
        updater.post(mAgentBounds, navMeshCacheItem, playerTile, mWorldspace, changedTilesRemove);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_EQ(navMeshCacheItem->lockConst()->getImpl().getTileRefAt(0, 0, 0), 0u);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, should_stop_writing_to_db_when_size_limit_is_reached)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                addHeightFieldPlane(mRecastMeshManager, osg::Vec2i(x, y));
        addObject(mBox, mRecastMeshManager);
        auto db = std::make_unique<NavMeshDb>(":memory:", 4097);
        NavMeshDb* const dbPtr = db.get();
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db));
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        std::map<TilePosition, ChangeType> changedTiles;
        for (int x = -5; x <= 5; ++x)
            for (int y = -5; y <= 5; ++y)
                changedTiles.emplace(TilePosition{ x, y }, ChangeType::add);
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        updater.stop();

        std::size_t present = 0;

        for (int x = -5; x <= 5; ++x)
        {
            for (int y = -5; y <= 5; ++y)
            {
                const TilePosition tilePosition(x, y);
                const auto recastMesh = mRecastMeshManager.getMesh(mWorldspace, tilePosition);
                ASSERT_NE(recastMesh, nullptr);
                const auto objects = makeDbRefGeometryObjects(
                    recastMesh->getMeshSources(), [&](const MeshSource& v) { return resolveMeshSource(*dbPtr, v); });
                if (std::holds_alternative<MeshSource>(objects))
                    continue;
                present += dbPtr
                               ->findTile(mWorldspace, tilePosition,
                                   serialize(mSettings.mRecast, mAgentBounds, *recastMesh,
                                       std::get<std::vector<DbRefGeometryObject>>(objects)))
                               .has_value();
            }
        }

        EXPECT_EQ(present, 11);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, next_tile_id_should_be_updated_on_duplicate)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        addObject(mBox, mRecastMeshManager);
        auto db = std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max());
        NavMeshDb* const dbPtr = db.get();
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db));

        const TileId nextTileId(dbPtr->getMaxTileId() + 1);
        ASSERT_EQ(dbPtr->insertTile(nextTileId, mWorldspace, TilePosition{}, TileVersion{ 1 }, {}, {}), 1);

        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const TilePosition tilePosition{ 0, 0 };
        const std::map<TilePosition, ChangeType> changedTiles{ { tilePosition, ChangeType::add } };

        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);

        const AgentBounds agentBounds{ CollisionShapeType::Cylinder, { 29, 29, 66 } };
        updater.post(agentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);

        updater.stop();

        const auto recastMesh = mRecastMeshManager.getMesh(mWorldspace, tilePosition);
        ASSERT_NE(recastMesh, nullptr);
        ShapeId nextShapeId{ 1 };
        const std::vector<DbRefGeometryObject> objects = makeDbRefGeometryObjects(recastMesh->getMeshSources(),
            [&](const MeshSource& v) { return resolveMeshSource(*dbPtr, v, nextShapeId); });
        const auto tile = dbPtr->findTile(
            mWorldspace, tilePosition, serialize(mSettings.mRecast, agentBounds, *recastMesh, objects));
        ASSERT_TRUE(tile.has_value());
        EXPECT_EQ(tile->mTileId, 2);
        EXPECT_EQ(tile->mVersion, navMeshFormatVersion);
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, repeated_tile_updates_should_be_delayed)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);

        mSettings.mMaxTilesNumber = 9;
        mSettings.mMinUpdateInterval = std::chrono::milliseconds(250);

        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);

        std::map<TilePosition, ChangeType> changedTiles;

        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                changedTiles.emplace(TilePosition{ x, y }, ChangeType::update);

        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);

        updater.wait(WaitConditionType::allJobsDone, &mListener);

        {
            const AsyncNavMeshUpdaterStats stats = updater.getStats();
            EXPECT_EQ(stats.mJobs, 0);
            EXPECT_EQ(stats.mWaiting.mDelayed, 0);
        }

        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);

        {
            const AsyncNavMeshUpdaterStats stats = updater.getStats();
            EXPECT_EQ(stats.mJobs, 49);
            EXPECT_EQ(stats.mWaiting.mDelayed, 49);
        }

        updater.wait(WaitConditionType::allJobsDone, &mListener);

        {
            const AsyncNavMeshUpdaterStats stats = updater.getStats();
            EXPECT_EQ(stats.mJobs, 0);
            EXPECT_EQ(stats.mWaiting.mDelayed, 0);
        }
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, should_write_debug_recast_mesh)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteRecastMeshToFile = true;
        const std::filesystem::path dir = TestingOpenMW::outputDirPath("DetourNavigatorAsyncNavMeshUpdaterTest");
        mSettings.mRecastMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_TRUE(std::filesystem::exists(dir / "0.0.recastmesh.obj"));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, should_write_debug_recast_mesh_with_revision)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteRecastMeshToFile = true;
        mSettings.mEnableRecastMeshFileNameRevision = true;
        const std::filesystem::path dir = TestingOpenMW::outputDirPath("DetourNavigatorAsyncNavMeshUpdaterTest");
        mSettings.mRecastMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_TRUE(std::filesystem::exists(dir / "0.0.recastmesh.1.2.obj"));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, writing_recast_mesh_to_absent_file_should_not_fail_tile_generation)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteRecastMeshToFile = true;
        const std::filesystem::path dir = TestingOpenMW::outputDir() / "absent";
        mSettings.mRecastMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_NE(navMeshCacheItem->lockConst()->getImpl().getTileRefAt(0, 0, 0), 0u);
        EXPECT_FALSE(std::filesystem::exists(dir));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, should_write_debug_navmesh)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteNavMeshToFile = true;
        const std::filesystem::path dir = TestingOpenMW::outputDirPath("DetourNavigatorAsyncNavMeshUpdaterTest");
        mSettings.mNavMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_TRUE(std::filesystem::exists(dir / "all_tiles_navmesh.bin"));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, should_write_debug_navmesh_with_revision)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteNavMeshToFile = true;
        mSettings.mEnableNavMeshFileNameRevision = true;
        const std::filesystem::path dir = TestingOpenMW::outputDirPath("DetourNavigatorAsyncNavMeshUpdaterTest");
        mSettings.mNavMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_TRUE(std::filesystem::exists(dir / "all_tiles_navmesh.1.1.bin"));
    }

    TEST_F(DetourNavigatorAsyncNavMeshUpdaterTest, writing_navmesh_to_absent_file_should_not_fail_tile_generation)
    {
        mRecastMeshManager.setWorldspace(mWorldspace, nullptr);
        addHeightFieldPlane(mRecastMeshManager);
        mSettings.mEnableWriteNavMeshToFile = true;
        const std::filesystem::path dir = TestingOpenMW::outputDir() / "absent";
        mSettings.mNavMeshPathPrefix = Files::pathToUnicodeString(dir) + "/";
        Log(Debug::Verbose) << mSettings.mRecastMeshPathPrefix;
        AsyncNavMeshUpdater updater(mSettings, mRecastMeshManager, mOffMeshConnectionsManager, nullptr);
        const auto navMeshCacheItem = std::make_shared<GuardedNavMeshCacheItem>(1, mSettings);
        const std::map<TilePosition, ChangeType> changedTiles{ { TilePosition{ 0, 0 }, ChangeType::add } };
        updater.post(mAgentBounds, navMeshCacheItem, mPlayerTile, mWorldspace, changedTiles);
        updater.wait(WaitConditionType::allJobsDone, &mListener);
        EXPECT_NE(navMeshCacheItem->lockConst()->getImpl().getTileRefAt(0, 0, 0), 0u);
        EXPECT_FALSE(std::filesystem::exists(dir));
    }

    struct DetourNavigatorSpatialJobQueueTest : Test
    {
        const AgentBounds mAgentBounds{ CollisionShapeType::Aabb, osg::Vec3f(1, 1, 1) };
        const std::shared_ptr<GuardedNavMeshCacheItem> mNavMeshCacheItemPtr;
        const std::weak_ptr<GuardedNavMeshCacheItem> mNavMeshCacheItem = mNavMeshCacheItemPtr;
        const ESM::RefId mWorldspace = ESM::RefId::stringRefId("worldspace");
        const TilePosition mChangedTile{ 0, 0 };
        const std::chrono::steady_clock::time_point mProcessTime{};
        const TilePosition mPlayerTile{ 0, 0 };
        const int mMaxTiles = 9;
    };

    TEST_F(DetourNavigatorSpatialJobQueueTest, should_store_multiple_jobs_per_tile)
    {
        std::list<Job> jobs;
        SpatialJobQueue queue;

        const ESM::RefId worldspace1 = ESM::RefId::stringRefId("worldspace1");
        const ESM::RefId worldspace2 = ESM::RefId::stringRefId("worldspace2");

        queue.push(jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, worldspace1, mChangedTile, ChangeType::remove, mProcessTime));
        queue.push(jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, worldspace2, mChangedTile, ChangeType::update, mProcessTime));

        ASSERT_EQ(queue.size(), 2);

        const auto job1 = queue.pop(mChangedTile);
        ASSERT_TRUE(job1.has_value());
        EXPECT_EQ((*job1)->mWorldspace, worldspace1);

        const auto job2 = queue.pop(mChangedTile);
        ASSERT_TRUE(job2.has_value());
        EXPECT_EQ((*job2)->mWorldspace, worldspace2);

        EXPECT_EQ(queue.size(), 0);
    }

    struct DetourNavigatorJobQueueTest : DetourNavigatorSpatialJobQueueTest
    {
    };

    TEST_F(DetourNavigatorJobQueueTest, pop_should_return_nullptr_from_empty)
    {
        JobQueue queue;
        ASSERT_FALSE(queue.hasJob());
        ASSERT_FALSE(queue.pop(mPlayerTile).has_value());
    }

    TEST_F(DetourNavigatorJobQueueTest, push_on_change_type_remove_should_add_to_removing)
    {
        const std::chrono::steady_clock::time_point processTime{};

        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::remove, processTime);

        JobQueue queue;
        queue.push(job);

        EXPECT_EQ(queue.getStats().mRemoving, 1);
    }

    TEST_F(DetourNavigatorJobQueueTest, pop_should_return_last_removing)
    {
        std::list<Job> jobs;
        JobQueue queue;

        queue.push(jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(0, 0),
            ChangeType::remove, mProcessTime));
        queue.push(jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(1, 0),
            ChangeType::remove, mProcessTime));

        ASSERT_TRUE(queue.hasJob());
        const auto job = queue.pop(mPlayerTile);
        ASSERT_TRUE(job.has_value());
        EXPECT_EQ((*job)->mChangedTile, TilePosition(1, 0));
    }

    TEST_F(DetourNavigatorJobQueueTest, push_on_change_type_not_remove_should_add_to_updating)
    {
        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, mProcessTime);

        JobQueue queue;
        queue.push(job);

        EXPECT_EQ(queue.getStats().mUpdating, 1);
    }

    TEST_F(DetourNavigatorJobQueueTest, pop_should_return_nearest_to_player_tile)
    {
        std::list<Job> jobs;

        JobQueue queue;
        queue.push(jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(0, 0),
            ChangeType::update, mProcessTime));
        queue.push(jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(1, 0),
            ChangeType::update, mProcessTime));

        ASSERT_TRUE(queue.hasJob());
        const auto job = queue.pop(TilePosition(1, 0));
        ASSERT_TRUE(job.has_value());
        EXPECT_EQ((*job)->mChangedTile, TilePosition(1, 0));
    }

    TEST_F(DetourNavigatorJobQueueTest, push_on_processing_time_more_than_now_should_add_to_delayed)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point processTime = now + std::chrono::seconds(1);

        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, processTime);

        JobQueue queue;
        queue.push(job, now);

        EXPECT_EQ(queue.getStats().mDelayed, 1);
    }

    TEST_F(DetourNavigatorJobQueueTest, pop_should_return_when_delayed_job_is_ready)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point processTime = now + std::chrono::seconds(1);

        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, processTime);

        JobQueue queue;
        queue.push(job, now);

        ASSERT_FALSE(queue.hasJob(now));
        ASSERT_FALSE(queue.pop(mPlayerTile, now).has_value());

        ASSERT_TRUE(queue.hasJob(processTime));
        EXPECT_TRUE(queue.pop(mPlayerTile, processTime).has_value());
    }

    TEST_F(DetourNavigatorJobQueueTest, update_should_move_ready_delayed_to_updating)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point processTime = now + std::chrono::seconds(1);

        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, processTime);

        JobQueue queue;
        queue.push(job, now);

        ASSERT_EQ(queue.getStats().mDelayed, 1);

        queue.update(mPlayerTile, mMaxTiles, processTime);

        EXPECT_EQ(queue.getStats().mDelayed, 0);
        EXPECT_EQ(queue.getStats().mUpdating, 1);
    }

    TEST_F(DetourNavigatorJobQueueTest, update_should_move_ready_delayed_to_removing_when_out_of_range)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point processTime = now + std::chrono::seconds(1);

        std::list<Job> jobs;
        const JobIt job = jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, processTime);

        JobQueue queue;
        queue.push(job, now);

        ASSERT_EQ(queue.getStats().mDelayed, 1);

        queue.update(TilePosition(10, 10), mMaxTiles, processTime);

        EXPECT_EQ(queue.getStats().mDelayed, 0);
        EXPECT_EQ(queue.getStats().mRemoving, 1);
        EXPECT_EQ(job->mChangeType, ChangeType::remove);
    }

    TEST_F(DetourNavigatorJobQueueTest, update_should_move_updating_to_removing_when_out_of_range)
    {
        std::list<Job> jobs;

        JobQueue queue;
        queue.push(jobs.emplace(
            jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, mChangedTile, ChangeType::update, mProcessTime));
        queue.push(jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(10, 10),
            ChangeType::update, mProcessTime));

        ASSERT_EQ(queue.getStats().mUpdating, 2);

        queue.update(TilePosition(10, 10), mMaxTiles);

        EXPECT_EQ(queue.getStats().mUpdating, 1);
        EXPECT_EQ(queue.getStats().mRemoving, 1);
    }

    TEST_F(DetourNavigatorJobQueueTest, clear_should_remove_all)
    {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        const std::chrono::steady_clock::time_point processTime = now + std::chrono::seconds(1);

        std::list<Job> jobs;
        const JobIt removing = jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace,
            TilePosition(0, 0), ChangeType::remove, mProcessTime);
        const JobIt updating = jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace,
            TilePosition(1, 0), ChangeType::update, mProcessTime);
        const JobIt delayed = jobs.emplace(jobs.end(), mAgentBounds, mNavMeshCacheItem, mWorldspace, TilePosition(2, 0),
            ChangeType::update, processTime);

        JobQueue queue;
        queue.push(removing);
        queue.push(updating);
        queue.push(delayed, now);

        ASSERT_EQ(queue.getStats().mRemoving, 1);
        ASSERT_EQ(queue.getStats().mUpdating, 1);
        ASSERT_EQ(queue.getStats().mDelayed, 1);

        queue.clear();

        EXPECT_EQ(queue.getStats().mRemoving, 0);
        EXPECT_EQ(queue.getStats().mUpdating, 0);
        EXPECT_EQ(queue.getStats().mDelayed, 0);
    }
}
