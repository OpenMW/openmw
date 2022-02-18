#include "navmesh.hpp"

#include "worldspacedata.hpp"

#include <components/debug/debuglog.hpp>
#include <components/detournavigator/generatenavmeshtile.hpp>
#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/navmeshdb.hpp>
#include <components/detournavigator/navmeshdbutils.hpp>
#include <components/detournavigator/preparednavmeshdata.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/recastmeshprovider.hpp>
#include <components/detournavigator/serialization.hpp>
#include <components/detournavigator/tileposition.hpp>
#include <components/misc/progressreporter.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sqlite3/transaction.hpp>

#include <osg/Vec3f>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>
#include <random>
#include <string_view>

namespace NavMeshTool
{
    namespace
    {
        using DetourNavigator::GenerateNavMeshTile;
        using DetourNavigator::NavMeshDb;
        using DetourNavigator::NavMeshTileInfo;
        using DetourNavigator::PreparedNavMeshData;
        using DetourNavigator::RecastMeshProvider;
        using DetourNavigator::MeshSource;
        using DetourNavigator::Settings;
        using DetourNavigator::ShapeId;
        using DetourNavigator::TileId;
        using DetourNavigator::TilePosition;
        using DetourNavigator::TileVersion;
        using DetourNavigator::TilesPositionsRange;
        using Sqlite3::Transaction;

        void logGeneratedTiles(std::size_t provided, std::size_t expected)
        {
            Log(Debug::Info) << provided << "/" << expected << " ("
                << (static_cast<double>(provided) / static_cast<double>(expected) * 100)
                << "%) navmesh tiles are generated";
        }

        struct LogGeneratedTiles
        {
            void operator()(std::size_t provided, std::size_t expected) const
            {
                logGeneratedTiles(provided, expected);
            }
        };

        class NavMeshTileConsumer final : public DetourNavigator::NavMeshTileConsumer
        {
        public:
            std::atomic_size_t mExpected {0};

            explicit NavMeshTileConsumer(NavMeshDb&& db, bool removeUnusedTiles)
                : mDb(std::move(db))
                , mRemoveUnusedTiles(removeUnusedTiles)
                , mTransaction(mDb.startTransaction())
                , mNextTileId(mDb.getMaxTileId() + 1)
                , mNextShapeId(mDb.getMaxShapeId() + 1)
            {}

            std::size_t getProvided() const { return mProvided.load(); }

            std::size_t getInserted() const { return mInserted.load(); }

            std::size_t getUpdated() const { return mUpdated.load(); }

            std::size_t getDeleted() const
            {
                const std::lock_guard lock(mMutex);
                return mDeleted;
            }

            std::int64_t resolveMeshSource(const MeshSource& source) override
            {
                const std::lock_guard lock(mMutex);
                return DetourNavigator::resolveMeshSource(mDb, source, mNextShapeId);
            }

            std::optional<NavMeshTileInfo> find(std::string_view worldspace, const TilePosition &tilePosition,
                const std::vector<std::byte> &input) override
            {
                std::optional<NavMeshTileInfo> result;
                std::lock_guard lock(mMutex);
                if (const auto tile = mDb.findTile(worldspace, tilePosition, input))
                {
                    NavMeshTileInfo info;
                    info.mTileId = tile->mTileId;
                    info.mVersion = tile->mVersion;
                    result.emplace(info);
                }
                return result;
            }

            void ignore(std::string_view worldspace, const TilePosition& tilePosition) override
            {
                if (mRemoveUnusedTiles)
                {
                    std::lock_guard lock(mMutex);
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                }
                report();
            }

            void identity(std::string_view worldspace, const TilePosition& tilePosition, std::int64_t tileId) override
            {
                if (mRemoveUnusedTiles)
                {
                    std::lock_guard lock(mMutex);
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId {tileId}));
                }
                report();
            }

            void insert(std::string_view worldspace, const TilePosition& tilePosition,
                std::int64_t version, const std::vector<std::byte>& input, PreparedNavMeshData& data) override
            {
                if (mRemoveUnusedTiles)
                {
                    std::lock_guard lock(mMutex);
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                }
                data.mUserId = static_cast<unsigned>(mNextTileId);
                {
                    std::lock_guard lock(mMutex);
                    mDb.insertTile(mNextTileId, worldspace, tilePosition, TileVersion {version}, input, serialize(data));
                    ++mNextTileId.t;
                }
                ++mInserted;
                report();
            }

            void update(std::string_view worldspace, const TilePosition& tilePosition,
                std::int64_t tileId, std::int64_t version, PreparedNavMeshData& data) override
            {
                data.mUserId = static_cast<unsigned>(tileId);
                {
                    std::lock_guard lock(mMutex);
                    if (mRemoveUnusedTiles)
                        mDeleted += static_cast<std::size_t>(mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId {tileId}));
                    mDb.updateTile(TileId {tileId}, TileVersion {version}, serialize(data));
                }
                ++mUpdated;
                report();
            }

            void wait()
            {
                constexpr std::size_t tilesPerTransaction = 3000;
                std::unique_lock lock(mMutex);
                while (mProvided < mExpected)
                {
                    mHasTile.wait(lock);
                    if (mProvided % tilesPerTransaction == 0)
                    {
                        mTransaction.commit();
                        mTransaction = mDb.startTransaction();
                    }
                }
                logGeneratedTiles(mProvided, mExpected);
            }

            void commit() { mTransaction.commit(); }

            void vacuum() { mDb.vacuum(); }

            void removeTilesOutsideRange(std::string_view worldspace, const TilesPositionsRange& range)
            {
                const std::lock_guard lock(mMutex);
                mTransaction.commit();
                Log(Debug::Info) << "Removing tiles outside processed range for worldspace \"" << worldspace << "\"...";
                mDeleted += static_cast<std::size_t>(mDb.deleteTilesOutsideRange(worldspace, range));
                mTransaction = mDb.startTransaction();
            }

        private:
            std::atomic_size_t mProvided {0};
            std::atomic_size_t mInserted {0};
            std::atomic_size_t mUpdated {0};
            std::size_t mDeleted = 0;
            mutable std::mutex mMutex;
            NavMeshDb mDb;
            const bool mRemoveUnusedTiles;
            Transaction mTransaction;
            TileId mNextTileId;
            std::condition_variable mHasTile;
            Misc::ProgressReporter<LogGeneratedTiles> mReporter;
            ShapeId mNextShapeId;
            std::mutex mReportMutex;

            void report()
            {
                const std::size_t provided = mProvided.fetch_add(1, std::memory_order_relaxed) + 1;
                mReporter(provided, mExpected);
                mHasTile.notify_one();
            }
        };
    }

    void generateAllNavMeshTiles(const osg::Vec3f& agentHalfExtents, const Settings& settings,
        std::size_t threadsNumber, bool removeUnusedTiles, WorldspaceData& data, NavMeshDb&& db)
    {
        Log(Debug::Info) << "Generating navmesh tiles by " << threadsNumber << " parallel workers...";

        SceneUtil::WorkQueue workQueue(threadsNumber);
        auto navMeshTileConsumer = std::make_shared<NavMeshTileConsumer>(std::move(db), removeUnusedTiles);
        std::size_t tiles = 0;
        std::mt19937_64 random;

        for (const std::unique_ptr<WorldspaceNavMeshInput>& input : data.mNavMeshInputs)
        {
            const auto range = DetourNavigator::makeTilesPositionsRange(
                Misc::Convert::toOsgXY(input->mAabb.m_min),
                Misc::Convert::toOsgXY(input->mAabb.m_max),
                settings.mRecast
            );

            if (removeUnusedTiles)
                navMeshTileConsumer->removeTilesOutsideRange(input->mWorldspace, range);

            std::vector<TilePosition> worldspaceTiles;

            DetourNavigator::getTilesPositions(range,
                [&] (const TilePosition& tilePosition) { worldspaceTiles.push_back(tilePosition); });

            tiles += worldspaceTiles.size();

            navMeshTileConsumer->mExpected = tiles;

            std::shuffle(worldspaceTiles.begin(), worldspaceTiles.end(), random);

            for (const TilePosition& tilePosition : worldspaceTiles)
                workQueue.addWorkItem(new GenerateNavMeshTile(
                    input->mWorldspace,
                    tilePosition,
                    RecastMeshProvider(input->mTileCachedRecastMeshManager),
                    agentHalfExtents,
                    settings,
                    navMeshTileConsumer
                ));
        }

        navMeshTileConsumer->wait();
        navMeshTileConsumer->commit();

        const auto inserted = navMeshTileConsumer->getInserted();
        const auto updated = navMeshTileConsumer->getUpdated();
        const auto deleted = navMeshTileConsumer->getDeleted();

        Log(Debug::Info) << "Generated navmesh for " << navMeshTileConsumer->getProvided() << " tiles, "
            << inserted << " are inserted, "
            << updated << " updated and "
            << deleted << " deleted";

        if (inserted + updated + deleted > 0)
        {
            Log(Debug::Info) << "Vacuuming the database...";
            navMeshTileConsumer->vacuum();
        }
    }
}
