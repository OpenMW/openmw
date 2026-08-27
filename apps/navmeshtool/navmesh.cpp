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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <random>
#include <string_view>
#include <unordered_map>
#include <utility>
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

        // Limits pending tiles between worldspaces.
        constexpr std::size_t maxTilesInFlight = 4096;
        constexpr std::chrono::seconds transactionInterval(1);

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

        struct WorldspaceProgress
        {
            std::size_t mExpected = 0;
            std::size_t mProvided = 0;
            std::shared_ptr<RecastMeshProvider> mProvider;
        };
    }

    class NavMeshTileConsumer final : public DetourNavigator::NavMeshTileConsumer
    {
    public:
        explicit NavMeshTileConsumer(NavMeshDb& db, const GenerateAllNavMeshTilesOptions& options)
            : mDb(db)
            , mRemoveUnusedTiles(options.mRemoveUnusedTiles)
            , mWriteBinaryLog(options.mWriteBinaryLog)
            , mCollectStats(options.mCollectStats)
            , mTransaction(mDb.startTransaction(Sqlite3::TransactionMode::Immediate))
            , mNextTileId(mDb.getMaxTileId() + 1)
            , mNextShapeId(mDb.getMaxShapeId() + 1)
            , mLastCommit(std::chrono::steady_clock::now())
        {
        }

        Status getStatus() const { return mStatus.load(); }

        void expect(ESM::RefId worldspace, std::size_t tiles, std::shared_ptr<RecastMeshProvider> provider)
        {
            {
                const std::lock_guard lock(mProgressMutex);
                mProgress.emplace(
                    worldspace, WorldspaceProgress{ .mExpected = tiles, .mProvider = std::move(provider) });
            }
            const std::size_t expected = mExpected += tiles;
            if (mWriteBinaryLog)
                serializeToStderr(ExpectedTiles{ static_cast<std::uint64_t>(expected) });
        }

        std::int64_t resolveMeshSource(const MeshSource& source) override
        {
            const std::lock_guard lock(mDbMutex);
            const std::int64_t shapeId = DetourNavigator::resolveMeshSource(mDb, source, mNextShapeId);
            commitIfNeeded();
            return shapeId;
        }

        std::optional<NavMeshTileInfo> find(
            ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input) override
        {
            std::optional<NavMeshTileInfo> result;
            const std::lock_guard lock(mDbMutex);
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
                const std::lock_guard lock(mDbMutex);
                mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                commitIfNeeded();
            }
            report(worldspace);
        }

        void identity(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId) override
        {
            if (mRemoveUnusedTiles)
            {
                const std::lock_guard lock(mDbMutex);
                mDeleted
                    += static_cast<std::size_t>(mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId{ tileId }));
                commitIfNeeded();
            }
            report(worldspace);
        }

        void insert(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t version,
            const std::vector<std::byte>& input, PreparedNavMeshData& data) override
        {
            {
                const std::lock_guard lock(mDbMutex);
                if (mRemoveUnusedTiles)
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(worldspace, tilePosition));
                data.mUserId = static_cast<unsigned>(mNextTileId);
                mDb.insertTile(mNextTileId, worldspace, tilePosition, TileVersion{ version }, input, serialize(data));
                ++mNextTileId;
                commitIfNeeded();
            }
            ++mInserted;
            report(worldspace);
        }

        void update(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId, std::int64_t version,
            PreparedNavMeshData& data) override
        {
            data.mUserId = static_cast<unsigned>(tileId);
            {
                const std::lock_guard lock(mDbMutex);
                if (mRemoveUnusedTiles)
                {
                    mDeleted += static_cast<std::size_t>(
                        mDb.deleteTilesAtExcept(worldspace, tilePosition, TileId{ tileId }));
                }
                mDb.updateTile(TileId{ tileId }, TileVersion{ version }, serialize(data));
                commitIfNeeded();
            }
            ++mUpdated;
            report(worldspace);
        }

        void cancel(std::string_view reason) override
        {
            Status expected = Status::Ok;
            const Status status = reason.find("database or disk is full") != std::string_view::npos
                ? Status::NotEnoughSpace
                : Status::Cancelled;
            if (!mStatus.compare_exchange_strong(expected, status))
                return;
            // Predicate lock prevents missed cancellation wake-ups.
            {
                const std::lock_guard lock(mProvidedMutex);
                mProvidedChanged.notify_all();
            }
        }

        void updateStats(const NavMeshTileConsumerStats& value) override
        {
            const Misc::Locked<GenerateTilesStats> stats = mStats.lock();
            stats->mMaxPolyCountPerTile = std::max(stats->mMaxPolyCountPerTile, value.mPolyCount);
        }

        void removeTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range)
        {
            Log(Debug::Info) << "Removing tiles outside processed range for worldspace " << worldspace << "...";
            const std::lock_guard lock(mDbMutex);
            mDeleted += static_cast<std::size_t>(mDb.deleteTilesOutsideRange(worldspace, range));
            commitIfNeeded();
        }

        void waitTilesInFlight(std::size_t limit)
        {
            std::unique_lock lock(mProvidedMutex);
            mProvidedChanged.wait(lock, [&] { return mExpected - mProvided <= limit || mStatus != Status::Ok; });
        }

        GenerateTilesResult finish()
        {
            {
                std::unique_lock lock(mProvidedMutex);
                mProvidedChanged.wait(lock, [&] { return mProvided >= mExpected || mStatus != Status::Ok; });
            }
            if (mExpected > 0)
                logGeneratedTiles(mProvided, mExpected);
            if (mWriteBinaryLog)
                logGeneratedTilesMessage(mProvided);
            if (mStatus == Status::Ok)
            {
                const std::lock_guard lock(mDbMutex);
                mTransaction.commit();
            }
            return GenerateTilesResult{
                .mStatus = mStatus.load(),
                .mProvided = mProvided.load(),
                .mInserted = mInserted.load(),
                .mUpdated = mUpdated.load(),
                .mDeleted = mDeleted.load(),
                .mStats = *mStats.lockConst(),
            };
        }

    private:
        std::atomic_size_t mExpected{ 0 };
        std::atomic_size_t mProvided{ 0 };
        std::atomic_size_t mInserted{ 0 };
        std::atomic_size_t mUpdated{ 0 };
        std::atomic_size_t mDeleted{ 0 };
        NavMeshDb& mDb;
        const bool mRemoveUnusedTiles;
        const bool mWriteBinaryLog;
        const bool mCollectStats;
        std::mutex mDbMutex;
        Transaction mTransaction;
        TileId mNextTileId;
        ShapeId mNextShapeId;
        std::chrono::steady_clock::time_point mLastCommit;
        std::mutex mProgressMutex;
        std::unordered_map<ESM::RefId, WorldspaceProgress> mProgress;
        Misc::ProgressReporter<LogGeneratedTiles> mReporter;
        Misc::ScopeGuarded<GenerateTilesStats> mStats;
        std::mutex mProvidedMutex;
        std::condition_variable mProvidedChanged;
        std::atomic<Status> mStatus{ Status::Ok };

        void report(ESM::RefId worldspace)
        {
            std::size_t provided;
            {
                // Predicate lock prevents missed wake-ups.
                const std::lock_guard lock(mProvidedMutex);
                provided = mProvided.fetch_add(1, std::memory_order_relaxed) + 1;
                mProvidedChanged.notify_all();
            }
            mReporter(provided, mExpected);
            {
                const std::lock_guard lock(mProgressMutex);
                const auto it = mProgress.find(worldspace);
                if (++it->second.mProvided == it->second.mExpected)
                {
                    Log(Debug::Info) << "Generated " << it->second.mExpected << " navmesh tiles for " << worldspace
                                     << " worldspace";
                    mProgress.erase(it);
                }
            }
            if (mWriteBinaryLog)
                logGeneratedTilesMessage(provided);
        }

        void commitIfNeeded()
        {
            const auto now = std::chrono::steady_clock::now();
            if (now - mLastCommit > transactionInterval)
            {
                mTransaction.commit();
                mTransaction = mDb.startTransaction(Sqlite3::TransactionMode::Immediate);
                mLastCommit = now;
            }
        }
    };

    NavMeshTilesGenerator::NavMeshTilesGenerator(const AgentBounds& agentBounds, const Settings& settings,
        const GenerateAllNavMeshTilesOptions& options, NavMeshDb& db, SceneUtil::WorkQueue& workQueue)
        : mAgentBounds(agentBounds)
        , mSettings(settings)
        , mOptions(options)
        , mWorkQueue(workQueue)
        , mConsumer(std::make_shared<NavMeshTileConsumer>(db, options))
    {
    }

    Status NavMeshTilesGenerator::addWorldspace(WorldspaceData&& data)
    {
        mConsumer->waitTilesInFlight(maxTilesInFlight);

        if (mConsumer->getStatus() != Status::Ok)
            return mConsumer->getStatus();

        if (mOptions.mRemoveUnusedTiles)
        {
            const TilesPositionsRange range = DetourNavigator::makeTilesPositionsRange(
                Misc::Convert::toOsgXY(data.mAabb.m_min), Misc::Convert::toOsgXY(data.mAabb.m_max), mSettings.mRecast);
            mConsumer->removeTilesOutsideRange(data.mWorldspace, range);
        }

        std::vector<TilePosition> tiles = std::move(data.mTiles);

        {
            std::mt19937_64 random;
            std::shuffle(tiles.begin(), tiles.end(), random);
        }

        Log(Debug::Info) << "Queued " << tiles.size() << " navmesh tiles for " << data.mWorldspace << " worldspace";

        if (tiles.empty())
            return mConsumer->getStatus();

        const std::shared_ptr<RecastMeshProvider> recastMeshProvider
            = std::make_shared<RecastMeshProvider>(std::move(data.mTilesData));

        mConsumer->expect(data.mWorldspace, tiles.size(), recastMeshProvider);

        for (const TilePosition& tilePosition : tiles)
            mWorkQueue.addWorkItem(new GenerateNavMeshTile(data.mWorldspace, tilePosition, recastMeshProvider,
                mAgentBounds, mSettings, mOptions.mCollectStats, mConsumer));

        return mConsumer->getStatus();
    }

    GenerateTilesResult NavMeshTilesGenerator::finish()
    {
        return mConsumer->finish();
    }
}
