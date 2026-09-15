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
#include <exception>
#include <mutex>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
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
        constexpr std::size_t maxQueuedDbJobs = 1024;
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

        struct InsertTileJob
        {
            TileId mTileId;
            ESM::RefId mWorldspace;
            TilePosition mTilePosition;
            TileVersion mVersion;
            std::vector<std::byte> mInput;
            std::vector<std::byte> mData;
        };

        struct UpdateTileJob
        {
            TileId mTileId;
            ESM::RefId mWorldspace;
            TilePosition mTilePosition;
            TileVersion mVersion;
            std::vector<std::byte> mData;
        };

        struct DeleteTilesAtJob
        {
            ESM::RefId mWorldspace;
            TilePosition mTilePosition;
        };

        struct DeleteTilesAtExceptJob
        {
            ESM::RefId mWorldspace;
            TilePosition mTilePosition;
            TileId mTileId;
        };

        struct DeleteTilesOutsideRangeJob
        {
            ESM::RefId mWorldspace;
            TilesPositionsRange mRange;
        };

        using DbJob = std::variant<InsertTileJob, UpdateTileJob, DeleteTilesAtJob, DeleteTilesAtExceptJob,
            DeleteTilesOutsideRangeJob>;

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

        struct DbResult
        {
            std::size_t mInserted;
            std::size_t mUpdated;
            std::size_t mDeleted;
        };

        // Lookups block; one thread writes queued jobs.
        class NavMeshDbWriter
        {
        public:
            NavMeshDbWriter(NavMeshDb& db, const GenerateAllNavMeshTilesOptions& options,
                DetourNavigator::NavMeshTileConsumer& consumer, const std::atomic<Status>& status)
                : mDb(db)
                , mRemoveUnusedTiles(options.mRemoveUnusedTiles)
                , mCollectStats(options.mCollectStats)
                , mConsumer(consumer)
                , mStatus(status)
                , mTransaction(mDb.startTransaction(Sqlite3::TransactionMode::Immediate))
                , mNextTileId(mDb.getMaxTileId() + 1)
                , mNextShapeId(mDb.getMaxShapeId() + 1)
                , mWriter([this] { runWriter(); })
            {
            }

            ~NavMeshDbWriter()
            {
                {
                    const std::lock_guard lock(mQueueMutex);
                    mStopWriter = true;
                    mQueueNotEmpty.notify_all();
                }
                mWriter.join();
            }

            std::int64_t resolveMeshSource(const MeshSource& source)
            {
                const std::lock_guard lock(mDbMutex);
                return DetourNavigator::resolveMeshSource(mDb, source, mNextShapeId);
            }

            std::optional<NavMeshTileInfo> find(
                ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input)
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

            void ignore(ESM::RefId worldspace, const TilePosition& tilePosition)
            {
                if (mRemoveUnusedTiles)
                    push(DeleteTilesAtJob{ worldspace, tilePosition });
            }

            void identity(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId)
            {
                if (mRemoveUnusedTiles)
                    push(DeleteTilesAtExceptJob{ worldspace, tilePosition, TileId{ tileId } });
            }

            void insert(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t version,
                const std::vector<std::byte>& input, PreparedNavMeshData& data)
            {
                TileId tileId;
                {
                    const std::lock_guard lock(mQueueMutex);
                    tileId = mNextTileId;
                    ++mNextTileId;
                }
                data.mUserId = static_cast<unsigned>(tileId);
                push(InsertTileJob{
                    .mTileId = tileId,
                    .mWorldspace = worldspace,
                    .mTilePosition = tilePosition,
                    .mVersion = TileVersion{ version },
                    .mInput = input,
                    .mData = serialize(data),
                });
            }

            void update(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId,
                std::int64_t version, PreparedNavMeshData& data)
            {
                data.mUserId = static_cast<unsigned>(tileId);
                push(UpdateTileJob{
                    .mTileId = TileId{ tileId },
                    .mWorldspace = worldspace,
                    .mTilePosition = tilePosition,
                    .mVersion = TileVersion{ version },
                    .mData = serialize(data),
                });
            }

            void cancel()
            {
                // Predicate lock prevents missed cancellation wake-ups.
                {
                    const std::lock_guard lock(mQueueMutex);
                    mQueueNotFull.notify_all();
                    mQueueDrained.notify_all();
                }
            }

            void removeTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range)
            {
                push(DeleteTilesOutsideRangeJob{ worldspace, range });
            }

            DbResult finish()
            {
                {
                    std::unique_lock lock(mQueueMutex);
                    mQueueDrained.wait(lock, [&] { return (mQueue.empty() && !mWriterBusy) || mStatus != Status::Ok; });
                }
                if (mStatus == Status::Ok)
                {
                    const std::lock_guard lock(mDbMutex);
                    mTransaction.commit();
                }
                return DbResult{
                    .mInserted = mInserted.load(),
                    .mUpdated = mUpdated.load(),
                    .mDeleted = mDeleted.load(),
                };
            }

        private:
            std::atomic_size_t mInserted{ 0 };
            std::atomic_size_t mUpdated{ 0 };
            std::atomic_size_t mDeleted{ 0 };
            NavMeshDb& mDb;
            const bool mRemoveUnusedTiles;
            const bool mCollectStats;
            DetourNavigator::NavMeshTileConsumer& mConsumer;
            const std::atomic<Status>& mStatus;
            std::mutex mDbMutex;
            Transaction mTransaction;
            TileId mNextTileId;
            ShapeId mNextShapeId;
            std::mutex mQueueMutex;
            std::condition_variable mQueueNotEmpty;
            std::condition_variable mQueueNotFull;
            std::condition_variable mQueueDrained;
            std::vector<DbJob> mQueue;
            bool mWriterBusy = false;
            bool mStopWriter = false;
            std::thread mWriter;

            void push(DbJob&& job)
            {
                std::unique_lock lock(mQueueMutex);
                mQueueNotFull.wait(lock, [&] { return mQueue.size() < maxQueuedDbJobs || mStatus != Status::Ok; });
                if (mStatus != Status::Ok)
                    return;
                mQueue.push_back(std::move(job));
                mQueueNotEmpty.notify_one();
            }

            void execute(InsertTileJob& job)
            {
                if (mRemoveUnusedTiles)
                    mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(job.mWorldspace, job.mTilePosition));
                mDb.insertTile(job.mTileId, job.mWorldspace, job.mTilePosition, job.mVersion, job.mInput, job.mData);
                ++mInserted;
            }

            void execute(UpdateTileJob& job)
            {
                if (mRemoveUnusedTiles)
                {
                    mDeleted += static_cast<std::size_t>(
                        mDb.deleteTilesAtExcept(job.mWorldspace, job.mTilePosition, job.mTileId));
                }
                mDb.updateTile(job.mTileId, job.mVersion, job.mData);
                ++mUpdated;
            }

            void execute(DeleteTilesAtJob& job)
            {
                mDeleted += static_cast<std::size_t>(mDb.deleteTilesAt(job.mWorldspace, job.mTilePosition));
            }

            void execute(DeleteTilesAtExceptJob& job)
            {
                mDeleted += static_cast<std::size_t>(
                    mDb.deleteTilesAtExcept(job.mWorldspace, job.mTilePosition, job.mTileId));
            }

            void execute(DeleteTilesOutsideRangeJob& job)
            {
                Log(Debug::Info) << "Removing tiles outside processed range for worldspace " << job.mWorldspace
                                 << "...";
                mDeleted += static_cast<std::size_t>(mDb.deleteTilesOutsideRange(job.mWorldspace, job.mRange));
            }

            void runWriter()
            {
                std::vector<DbJob> batch;
                auto lastCommit = std::chrono::steady_clock::now();
                while (true)
                {
                    {
                        std::unique_lock lock(mQueueMutex);
                        mQueueNotEmpty.wait(lock, [&] { return !mQueue.empty() || mStopWriter; });
                        if (mQueue.empty())
                            break;
                        batch.swap(mQueue);
                        mWriterBusy = mStatus == Status::Ok;
                        if (!mWriterBusy)
                            batch.clear();
                        mQueueNotFull.notify_all();
                    }
                    if (batch.empty())
                        continue;
                    try
                    {
                        const std::lock_guard lock(mDbMutex);
                        for (DbJob& job : batch)
                            std::visit([this](auto& value) { execute(value); }, job);
                        const auto now = std::chrono::steady_clock::now();
                        if (now - lastCommit > transactionInterval)
                        {
                            mTransaction.commit();
                            mTransaction = mDb.startTransaction(Sqlite3::TransactionMode::Immediate);
                            lastCommit = now;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        Log(Debug::Warning) << "Failed to store navmesh tiles: " << e.what();
                        mConsumer.cancel(e.what());
                    }
                    batch.clear();
                    {
                        const std::lock_guard lock(mQueueMutex);
                        mWriterBusy = false;
                        mQueueDrained.notify_all();
                    }
                }
            }
        };
    }

    class NavMeshTileConsumer final : public DetourNavigator::NavMeshTileConsumer
    {
    public:
        explicit NavMeshTileConsumer(NavMeshDb& db, const GenerateAllNavMeshTilesOptions& options)
            : mWriteBinaryLog(options.mWriteBinaryLog)
            , mWriter(db, options, *this, mStatus)
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

        std::int64_t resolveMeshSource(const MeshSource& source) override { return mWriter.resolveMeshSource(source); }

        std::optional<NavMeshTileInfo> find(
            ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input) override
        {
            return mWriter.find(worldspace, tilePosition, input);
        }

        void ignore(ESM::RefId worldspace, const TilePosition& tilePosition) override
        {
            mWriter.ignore(worldspace, tilePosition);
            report(worldspace);
        }

        void identity(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId) override
        {
            mWriter.identity(worldspace, tilePosition, tileId);
            report(worldspace);
        }

        void insert(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t version,
            const std::vector<std::byte>& input, PreparedNavMeshData& data) override
        {
            mWriter.insert(worldspace, tilePosition, version, input, data);
            report(worldspace);
        }

        void update(ESM::RefId worldspace, const TilePosition& tilePosition, std::int64_t tileId, std::int64_t version,
            PreparedNavMeshData& data) override
        {
            mWriter.update(worldspace, tilePosition, tileId, version, data);
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
            mWriter.cancel();
        }

        void updateStats(const NavMeshTileConsumerStats& value) override
        {
            const Misc::Locked<GenerateTilesStats> stats = mStats.lock();
            stats->mMaxPolyCountPerTile = std::max(stats->mMaxPolyCountPerTile, value.mPolyCount);
        }

        void removeTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range)
        {
            mWriter.removeTilesOutsideRange(worldspace, range);
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
            const DbResult result = mWriter.finish();
            if (mExpected > 0)
                logGeneratedTiles(mProvided, mExpected);
            if (mWriteBinaryLog)
                logGeneratedTilesMessage(mProvided);
            return GenerateTilesResult{
                .mStatus = mStatus.load(),
                .mProvided = mProvided.load(),
                .mInserted = result.mInserted,
                .mUpdated = result.mUpdated,
                .mDeleted = result.mDeleted,
                .mStats = *mStats.lockConst(),
            };
        }

    private:
        std::atomic_size_t mExpected{ 0 };
        std::atomic_size_t mProvided{ 0 };
        const bool mWriteBinaryLog;
        std::mutex mProgressMutex;
        std::unordered_map<ESM::RefId, WorldspaceProgress> mProgress;
        Misc::ProgressReporter<LogGeneratedTiles> mReporter;
        Misc::ScopeGuarded<GenerateTilesStats> mStats;
        std::mutex mProvidedMutex;
        std::condition_variable mProvidedChanged;
        std::atomic<Status> mStatus{ Status::Ok };
        // Writer stops before progress state destruction.
        NavMeshDbWriter mWriter;

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

        // Tile order improves database locality.
        const std::vector<TilePosition> tiles = std::move(data.mTiles);

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
