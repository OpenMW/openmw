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
        using DetourNavigator::NavMeshTileInfo;
        using DetourNavigator::PreparedNavMeshData;
        using DetourNavigator::RecastMeshProvider;
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

            explicit NavMeshTileConsumer(NavMeshDb& db, bool removeUnusedTiles, bool writeBinaryLog)
                : mDb(db)
                , mRemoveUnusedTiles(removeUnusedTiles)
                , mWriteBinaryLog(writeBinaryLog)
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
                if (const auto tile = mDb.findTile(worldspace, tilePosition, input))
                {
                    NavMeshTileInfo info;
                    info.mTileId = tile->mTileId;
                    info.mVersion = tile->mVersion;
                    result.emplace(info);
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
                if (reason.find("database or disk is full") != std::string_view::npos)
                    mStatus = Status::NotEnoughSpace;
                else
                    mStatus = Status::Cancelled;
                mHasTile.notify_one();
            }

            Status wait()
            {
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
                if (mWriteBinaryLog)
                    logGeneratedTilesMessage(provided);
            }
        };
    }

    Result generateAllNavMeshTiles(const AgentBounds& agentBounds, const Settings& settings, bool removeUnusedTiles,
        bool writeBinaryLog, const WorldspaceData& data, NavMeshDb& db, SceneUtil::WorkQueue& workQueue)
    {
        Log(Debug::Info) << "Generating navmesh tiles for " << data.mWorldspace << " worldspace...";

        auto navMeshTileConsumer = std::make_shared<NavMeshTileConsumer>(db, removeUnusedTiles, writeBinaryLog);

        const auto range = DetourNavigator::makeTilesPositionsRange(
            Misc::Convert::toOsgXY(data.mAabb.m_min), Misc::Convert::toOsgXY(data.mAabb.m_max), settings.mRecast);

        if (removeUnusedTiles)
            navMeshTileConsumer->removeTilesOutsideRange(data.mWorldspace, range);

        std::vector<TilePosition> worldspaceTiles = data.mTiles;

        {
            const std::size_t tiles = worldspaceTiles.size();

            if (writeBinaryLog)
                serializeToStderr(ExpectedTiles{ static_cast<std::uint64_t>(tiles) });

            navMeshTileConsumer->mExpected = tiles;
        }

        {
            std::mt19937_64 random;
            std::shuffle(worldspaceTiles.begin(), worldspaceTiles.end(), random);
        }

        for (const TilePosition& tilePosition : worldspaceTiles)
            workQueue.addWorkItem(new GenerateNavMeshTile(data.mWorldspace, tilePosition,
                RecastMeshProvider(*data.mTileCachedRecastMeshManager), agentBounds, settings, navMeshTileConsumer));

        const Status status = navMeshTileConsumer->wait();
        if (status == Status::Ok)
            navMeshTileConsumer->commit();

        const auto inserted = navMeshTileConsumer->getInserted();
        const auto updated = navMeshTileConsumer->getUpdated();
        const auto deleted = navMeshTileConsumer->getDeleted();

        Log(Debug::Info) << "Generated navmesh for " << navMeshTileConsumer->getProvided() << " tiles, " << inserted
                         << " are inserted, " << updated << " updated and " << deleted << " deleted";

        return Result{
            .mStatus = status,
            .mNeedVacuum = inserted + updated + deleted > 0,
        };
    }
}
