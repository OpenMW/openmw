#include "navmesh.hpp"

#include "worldspacedata.hpp"

#include <components/bullethelpers/aabb.hpp>
#include <components/debug/debuglog.hpp>
#include <components/detournavigator/generatenavmeshtile.hpp>
#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/navmeshdb.hpp>
#include <components/detournavigator/navmeshdbutils.hpp>
#include <components/detournavigator/offmeshconnection.hpp>
#include <components/detournavigator/offmeshconnectionsmanager.hpp>
#include <components/detournavigator/preparednavmeshdata.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/recastmeshprovider.hpp>
#include <components/detournavigator/serialization.hpp>
#include <components/detournavigator/tilecachedrecastmeshmanager.hpp>
#include <components/detournavigator/tileposition.hpp>
#include <components/esm/loadcell.hpp>
#include <components/misc/guarded.hpp>
#include <components/misc/progressreporter.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/sqlite3/transaction.hpp>

#include <DetourNavMesh.h>

#include <osg/Vec3f>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

            explicit NavMeshTileConsumer(NavMeshDb db)
                : mDb(std::move(db))
                , mTransaction(mDb.startTransaction())
                , mNextTileId(mDb.getMaxTileId() + 1)
                , mNextShapeId(mDb.getMaxShapeId() + 1)
            {}

            std::size_t getProvided() const { return mProvided.load(); }

            std::size_t getInserted() const { return mInserted.load(); }

            std::size_t getUpdated() const { return mUpdated.load(); }

            std::int64_t resolveMeshSource(const MeshSource& source) override
            {
                const std::lock_guard lock(mMutex);
                return DetourNavigator::resolveMeshSource(mDb, source, mNextShapeId);
            }

            std::optional<NavMeshTileInfo> find(const std::string& worldspace, const TilePosition &tilePosition,
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

            void ignore() override { report(); }

            void insert(const std::string& worldspace, const TilePosition& tilePosition, std::int64_t version,
                const std::vector<std::byte>& input, PreparedNavMeshData& data) override
            {
                data.mUserId = static_cast<unsigned>(mNextTileId);
                {
                    std::lock_guard lock(mMutex);
                    mDb.insertTile(mNextTileId, worldspace, tilePosition, TileVersion {version}, input, serialize(data));
                    ++mNextTileId.t;
                }
                ++mInserted;
                report();
            }

            void update(std::int64_t tileId, std::int64_t version, PreparedNavMeshData& data) override
            {
                data.mUserId = static_cast<unsigned>(tileId);
                {
                    std::lock_guard lock(mMutex);
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

        private:
            std::atomic_size_t mProvided {0};
            std::atomic_size_t mInserted {0};
            std::atomic_size_t mUpdated {0};
            std::mutex mMutex;
            NavMeshDb mDb;
            Transaction mTransaction;
            TileId mNextTileId;
            std::condition_variable mHasTile;
            Misc::ProgressReporter<LogGeneratedTiles> mReporter;
            ShapeId mNextShapeId;

            void report()
            {
                mReporter(mProvided + 1, mExpected);
                ++mProvided;
                mHasTile.notify_one();
            }
        };
    }

    void generateAllNavMeshTiles(const osg::Vec3f& agentHalfExtents, const Settings& settings,
        const std::size_t threadsNumber, WorldspaceData& data, NavMeshDb&& db)
    {
        Log(Debug::Info) << "Generating navmesh tiles by " << threadsNumber << " parallel workers...";

        SceneUtil::WorkQueue workQueue(threadsNumber);
        auto navMeshTileConsumer = std::make_shared<NavMeshTileConsumer>(std::move(db));
        std::size_t tiles = 0;

        for (const std::unique_ptr<WorldspaceNavMeshInput>& input : data.mNavMeshInputs)
        {
            DetourNavigator::getTilesPositions(
                Misc::Convert::toOsg(input->mAabb.m_min), Misc::Convert::toOsg(input->mAabb.m_max), settings.mRecast,
                [&] (const TilePosition& tilePosition)
                {
                    workQueue.addWorkItem(new GenerateNavMeshTile(
                        input->mWorldspace,
                        tilePosition,
                        RecastMeshProvider(input->mTileCachedRecastMeshManager),
                        agentHalfExtents,
                        settings,
                        navMeshTileConsumer
                    ));

                    ++tiles;
                });

            navMeshTileConsumer->mExpected = tiles;
        }

        navMeshTileConsumer->wait();
        navMeshTileConsumer->commit();

        Log(Debug::Info) << "Generated navmesh for " << navMeshTileConsumer->getProvided() << " tiles, "
            << navMeshTileConsumer->getInserted() << " are inserted and "
            << navMeshTileConsumer->getUpdated() << " updated";
    }
}
