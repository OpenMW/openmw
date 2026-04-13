#include "navmesh.hpp"

#include "worldspacedata.hpp"

#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/detournavigator/generatenavmeshtile.hpp>
#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/navmeshdb.hpp>
#include <components/detournavigator/navmeshdbutils.hpp>
#include <components/detournavigator/preparednavmeshdata.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/recastmeshprovider.hpp>
#include <components/detournavigator/serialization.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/tileposition.hpp>
#include <components/misc/progressreporter.hpp>
#include <components/navmeshtool/protocol.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sqlite3/transaction.hpp>

#include <osg/Vec3f>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <random>
#include <string_view>
#include <vector>

namespace NavMeshTool
{
    namespace
    {
        using DetourNavigator::AgentBounds;
        using DetourNavigator::GenerateNavMeshTile;
        using DetourNavigator::MeshSource;
        using DetourNavigator::NavMeshDb;
        using DetourNavigator::NavMeshTileConsumerStats;
        using DetourNavigator::NavMeshTileInfo;
        using DetourNavigator::PreparedNavMeshData;
        using DetourNavigator::RecastMesh;
        using DetourNavigator::Settings;
        using DetourNavigator::ShapeId;
        using DetourNavigator::TileId;
        using DetourNavigator::TilePosition;
        using DetourNavigator::TilesPositionsRange;
        using DetourNavigator::TileVersion;
        using Sqlite3::Transaction;

        void logGeneratedTiles(std::size_t provided, std::size_t expected)
        {
            Log(Debug::Info) << provided << "/" << expected << " ("
                             << (static_cast<double>(provided) / static_cast<double>(expected) * 100)
                             << "%) navmesh tiles are generated";
        }

        template <class T>
        void serializeToStderr(const T& value)
        {
            const std::vector<std::byte> data = serialize(value);
            Debug::getLockedRawStderr()->write(
                reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }

        void logGeneratedTilesMessage(std::size_t number)
        {
            serializeToStderr(GeneratedTiles{ static_cast<std::uint64_t>(number) });
        }

        struct LogGeneratedTiles
        {
            void operator()(std::size_t provided, std::size_t expected) const { logGeneratedTiles(provided, expected); }
        };

        class NavMeshTileConsumer final : public DetourNavigator::NavMeshTileConsumer
        {
        public:
            std::atomic_size_t mExpected{ 0 };

            explicit NavMeshTileConsumer(NavMeshDb& db, const GenerateAllNavMeshTilesOptions& options)
                : mDb(db)
                , mRemoveUnusedTiles(options.mRemoveUnusedTiles)
                , mWriteBinaryLog(options.mWriteBinaryLog)
                , mCollectStats(options.mCollectStats)
                , mTransaction(mDb.startTransaction(Sqlite3::TransactionMode::Immediate))
                , mNextTileId(mDb.getMaxTileId() + 1)
                , mNextShapeId(mDb.getMaxShapeId() + 1)
            {
            }

            std::size_t getProvided() const { return mProvided.load(); }

            std::size_t getInserted() const { return mInserted.load(); }

            std::size_t getUpdated() const { return mUpdated.load(); }

            std::size_t getDeleted() const
            {
                const std::lock_guard lock(mMutex);
                return mDeleted;
            }

            GenerateTilesStats getStats() const { return *mStats.lockConst(); }

            std::int64_t resolveMeshSource(const MeshSource& source) override
            {
                const std::lock_guard lock(mMutex);
                return DetourNavigator::resolveMeshSource(mDb, source, mNextShapeId);
            }

            std::optional<NavMeshTileInfo> find(
                ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input) override
            {
                std::optional<NavMeshTileInfo> result;
                std::lock_guard lock(mMutex);
                if (mCollectStats)
                {
                    if (const auto tile = mDb.getTileData(worldspace, tilePosition, input))
                    {
                        NavMeshTileInfo info;
                        info.mTileId = tile->mTileId;
                        info.mVersion = tile->mVersion;
                        info.mData = std::make_unique<PreparedNavMeshData>();
                        deserialize(tile->mData, *info.mData);
                        result.emplace(std::move(info));
                    }
                }
                else
                {
                    if (const auto tile = mDb.findTile(worldspace, tilePosition, input))
                    {
                        NavMeshTileInfo info;
                        info.mTileId = tile->mTileId;
                        info.mVersion = tile->mVersion;
                        result.emplace(std::move(info));
                    }
                }
                return result;
            }

            void ignore(ESM::RefId worldspace, const TilePosition& tilePosition) override
            {
                if (mRemoveUnusedTiles)
                {
                    std::lock_guard lock(mMutex);
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                }
                report();
            }

            void identity(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId) override
            {
                if (mRemoveUnusedTiles)
                {
                    std::lock_guard lock(mMutex);
                    mDeleted += static_cast<std::size_t>(
                        mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId{ tileId }));
                }
                report();
            }

            void insert(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t version,
                const std::vector<std::byte>& input, PreparedNavMeshData& data) override
            {
                {
                    std::lock_guard lock(mMutex);
                    if (mRemoveUnusedTiles)
                        mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                    data.mUserId = static_cast<unsigned>(mNextTileId);
                    mDb.insertTile(
                        mNextTileId, worldspace, tilePosition, TileVersion{ version }, input, serialize(data));
                    ++mNextTileId;
                }
                ++mInserted;
                report();
            }

            void update(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId,
                std::int64_t version, PreparedNavMeshData& data) override
            {
                data.mUserId = static_cast<unsigned>(tileId);
                {
                    std::lock_guard lock(mMutex);
                    if (mRemoveUnusedTiles)
                        mDeleted += static_cast<std::size_t>(
                            mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId{ tileId }));
                    mDb.updateTile(TileId{ tileId }, TileVersion{ version }, serialize(data));
                }
                ++mUpdated;
                report();
            }

            void cancel(std::string_view reason) override
            {
                std::unique_lock lock(mMutex);
                if (mStatus != Status::Ok)
                    return;
                if (reason.find("database or disk is full") != std::string_view::npos)
                    mStatus = Status::NotEnoughSpace;
                else
                    mStatus = Status::Cancelled;
                mHasTile.notify_one();
            }

            void updateStats(const NavMeshTileConsumerStats& value) override
            {
                const Misc::Locked<GenerateTilesStats> stats = mStats.lock();
                stats->mMaxPolyCountPerTile = std::max(stats->mMaxPolyCountPerTile, value.mPolyCount);
            }

            Status wait()
            {
                if (mExpected == 0)
                    return Status::Ok;
                constexpr std::chrono::seconds transactionInterval(1);
                std::unique_lock lock(mMutex);
                auto start = std::chrono::steady_clock::now();
                while (mProvided < mExpected && mStatus == Status::Ok)
                {
                    mHasTile.wait(lock);
                    const auto now = std::chrono::steady_clock::now();
                    if (now - start > transactionInterval)
                    {
                        mTransaction.commit();
                        mTransaction = mDb.startTransaction(Sqlite3::TransactionMode::Immediate);
                        start = now;
                    }
                }
                logGeneratedTiles(mProvided, mExpected);
                if (mWriteBinaryLog)
                    logGeneratedTilesMessage(mProvided);
                return mStatus;
            }

            void commit()
            {
                const std::lock_guard lock(mMutex);
                mTransaction.commit();
            }

            void removeTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range)
            {
                const std::lock_guard lock(mMutex);
                mTransaction.commit();
                Log(Debug::Info) << "Removing tiles outside processed range for worldspace \"" << worldspace << "\"...";
                mDeleted += static_cast<std::size_t>(mDb.deleteTilesOutsideRange(worldspace, range));
                mTransaction = mDb.startTransaction(Sqlite3::TransactionMode::Immediate);
            }

        private:
            std::atomic_size_t mProvided{ 0 };
            std::atomic_size_t mInserted{ 0 };
            std::atomic_size_t mUpdated{ 0 };
            std::size_t mDeleted = 0;
            Status mStatus = Status::Ok;
            mutable std::mutex mMutex;
            NavMeshDb& mDb;
            const bool mRemoveUnusedTiles;
            const bool mWriteBinaryLog;
            const bool mCollectStats;
            Transaction mTransaction;
            TileId mNextTileId;
            std::condition_variable mHasTile;
            Misc::ProgressReporter<LogGeneratedTiles> mReporter;
            ShapeId mNextShapeId;
            std::mutex mReportMutex;
            Misc::ScopeGuarded<GenerateTilesStats> mStats;

            void report()
            {
                const std::size_t provided = mProvided.fetch_add(1, std::memory_order_relaxed) + 1;
                mReporter(provided, mExpected);
                mHasTile.notify_one();
                if (mWriteBinaryLog)
                    logGeneratedTilesMessage(provided);
            }
        };

        class RecastMeshProvider final : public DetourNavigator::RecastMeshProvider
        {
        public:
            explicit RecastMeshProvider(std::shared_ptr<TilesData> tilesData)
                : mTilesData(std::move(tilesData))
            {
            }

            std::shared_ptr<RecastMesh> getMesh(ESM::RefId worldspace, const TilePosition& tilePosition) const override
            {
                return mTilesData->mTileCachedRecastMeshManager.getNewMesh(worldspace, tilePosition);
            }

        private:
            std::shared_ptr<TilesData> mTilesData;
        };
    }

    GenerateTilesResult generateAllNavMeshTiles(const AgentBounds& agentBounds, const Settings& settings,
        const GenerateAllNavMeshTilesOptions& options, const WorldspaceData& data, NavMeshDb& db,
        SceneUtil::WorkQueue& workQueue)
    {
        Log(Debug::Info) << "Generating navmesh tiles for " << data.mWorldspace << " worldspace...";

        const std::shared_ptr<NavMeshTileConsumer> navMeshTileConsumer
            = std::make_shared<NavMeshTileConsumer>(db, options);

        const TilesPositionsRange range = DetourNavigator::makeTilesPositionsRange(
            Misc::Convert::toOsgXY(data.mAabb.m_min), Misc::Convert::toOsgXY(data.mAabb.m_max), settings.mRecast);

        if (options.mRemoveUnusedTiles)
            navMeshTileConsumer->removeTilesOutsideRange(data.mWorldspace, range);

        std::vector<TilePosition> worldspaceTiles = data.mTiles;

        {
            const std::size_t tiles = worldspaceTiles.size();

            if (options.mWriteBinaryLog)
                serializeToStderr(ExpectedTiles{ static_cast<std::uint64_t>(tiles) });

            navMeshTileConsumer->mExpected = tiles;
        }

        {
            std::mt19937_64 random;
            std::shuffle(worldspaceTiles.begin(), worldspaceTiles.end(), random);
        }

        const std::shared_ptr<RecastMeshProvider> recastMeshProvider
            = std::make_shared<RecastMeshProvider>(data.mTilesData);

        for (const TilePosition& tilePosition : worldspaceTiles)
            workQueue.addWorkItem(new GenerateNavMeshTile(data.mWorldspace, tilePosition, recastMeshProvider,
                agentBounds, settings, options.mCollectStats, navMeshTileConsumer));

        const Status status = navMeshTileConsumer->wait();
        if (status == Status::Ok)
            navMeshTileConsumer->commit();

        const std::size_t provided = navMeshTileConsumer->getProvided();
        const std::size_t inserted = navMeshTileConsumer->getInserted();
        const std::size_t updated = navMeshTileConsumer->getUpdated();
        const std::size_t deleted = navMeshTileConsumer->getDeleted();

        Log(Debug::Info) << "Generated navmesh for " << provided << " tiles: " << inserted << " inserted, " << updated
                         << " updated, " << deleted << " deleted";

        return GenerateTilesResult{
            .mStatus = status,
            .mProvided = provided,
            .mInserted = inserted,
            .mUpdated = updated,
            .mDeleted = deleted,
            .mStats = navMeshTileConsumer->getStats(),
        };
    }
}
