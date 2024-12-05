#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H

#include "agentbounds.hpp"
#include "changetype.hpp"
#include "guardednavmeshcacheitem.hpp"
#include "navmeshcacheitem.hpp"
#include "navmeshdb.hpp"
#include "navmeshtilescache.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "sharednavmeshcacheitem.hpp"
#include "stats.hpp"
#include "tilecachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "waitconditiontype.hpp"

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <iosfwd>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <tuple>

class dtNavMesh;

namespace Loading
{
    class Listener;
}

namespace DetourNavigator
{
    enum class JobState
    {
        Initial,
        WithDbResult,
    };

    struct Job
    {
        const std::size_t mId;
        const AgentBounds mAgentBounds;
        const std::weak_ptr<GuardedNavMeshCacheItem> mNavMeshCacheItem;
        const ESM::RefId mWorldspace;
        const TilePosition mChangedTile;
        std::chrono::steady_clock::time_point mProcessTime;
        ChangeType mChangeType;
        JobState mState = JobState::Initial;
        std::vector<std::byte> mInput;
        std::shared_ptr<RecastMesh> mRecastMesh;
        std::optional<TileData> mCachedTileData;
        std::unique_ptr<PreparedNavMeshData> mGeneratedNavMeshData;

        Job(const AgentBounds& agentBounds, std::weak_ptr<GuardedNavMeshCacheItem> navMeshCacheItem,
            ESM::RefId worldspace, const TilePosition& changedTile, ChangeType changeType,
            std::chrono::steady_clock::time_point processTime);
    };

    using JobIt = std::list<Job>::iterator;

    class SpatialJobQueue
    {
    public:
        std::size_t size() const { return mSize; }

        void clear();

        void push(JobIt job);

        std::optional<JobIt> pop(TilePosition playerTile);

        void update(TilePosition playerTile, int maxTiles, std::vector<JobIt>& removing);

    private:
        using IndexPoint = boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>;
        using UpdatingMap = std::map<TilePosition, std::deque<JobIt>>;
        using IndexValue = std::pair<IndexPoint, UpdatingMap::iterator>;

        std::size_t mSize = 0;
        UpdatingMap mValues;
        boost::geometry::index::rtree<IndexValue, boost::geometry::index::linear<4>> mIndex;
    };

    class JobQueue
    {
    public:
        JobQueueStats getStats() const
        {
            return JobQueueStats{
                .mRemoving = mRemoving.size(),
                .mUpdating = mUpdating.size(),
                .mDelayed = mDelayed.size(),
            };
        }

        bool hasJob(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const;

        void clear();

        void push(JobIt job, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        std::optional<JobIt> pop(
            TilePosition playerTile, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        void update(TilePosition playerTile, int maxTiles,
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

    private:
        std::vector<JobIt> mRemoving;
        SpatialJobQueue mUpdating;
        std::deque<JobIt> mDelayed;
    };

    enum class JobStatus
    {
        Done,
        Fail,
        MemoryCacheMiss,
    };

    std::ostream& operator<<(std::ostream& stream, JobStatus value);

    class DbJobQueue
    {
    public:
        void push(JobIt job);

        std::optional<JobIt> pop();

        void update(TilePosition playerTile);

        void stop();

        DbJobQueueStats getStats() const;

    private:
        mutable std::mutex mMutex;
        std::condition_variable mHasJob;
        SpatialJobQueue mReading;
        std::deque<JobIt> mWriting;
        TilePosition mPlayerTile;
        bool mShouldStop = false;
    };

    class AsyncNavMeshUpdater;

    class DbWorker
    {
    public:
        DbWorker(AsyncNavMeshUpdater& updater, std::unique_ptr<NavMeshDb>&& db, TileVersion version,
            const RecastSettings& recastSettings, bool writeToDb);

        ~DbWorker();

        DbWorkerStats getStats() const;

        void enqueueJob(JobIt job);

        void update(TilePosition playerTile) { mQueue.update(playerTile); }

        void stop();

    private:
        AsyncNavMeshUpdater& mUpdater;
        const RecastSettings& mRecastSettings;
        const std::unique_ptr<NavMeshDb> mDb;
        const TileVersion mVersion;
        bool mWriteToDb;
        TileId mNextTileId;
        ShapeId mNextShapeId;
        DbJobQueue mQueue;
        std::atomic_bool mShouldStop{ false };
        std::atomic_size_t mGetTileCount{ 0 };
        std::thread mThread;

        inline void run() noexcept;

        inline void processJob(JobIt job);

        inline void processReadingJob(JobIt job);

        inline void processWritingJob(JobIt job);
    };

    class AsyncNavMeshUpdater
    {
    public:
        AsyncNavMeshUpdater(const Settings& settings, TileCachedRecastMeshManager& recastMeshManager,
            OffMeshConnectionsManager& offMeshConnectionsManager, std::unique_ptr<NavMeshDb>&& db);
        ~AsyncNavMeshUpdater();

        void post(const AgentBounds& agentBounds, const SharedNavMeshCacheItem& navMeshCacheItem,
            const TilePosition& playerTile, ESM::RefId worldspace,
            const std::map<TilePosition, ChangeType>& changedTiles);

        void wait(WaitConditionType waitConditionType, Loading::Listener* listener);

        void stop();

        AsyncNavMeshUpdaterStats getStats() const;

        void enqueueJob(JobIt job);

        void removeJob(JobIt job);

    private:
        std::reference_wrapper<const Settings> mSettings;
        std::reference_wrapper<TileCachedRecastMeshManager> mRecastMeshManager;
        std::reference_wrapper<OffMeshConnectionsManager> mOffMeshConnectionsManager;
        std::atomic_bool mShouldStop;
        mutable std::mutex mMutex;
        std::condition_variable mHasJob;
        std::condition_variable mDone;
        std::condition_variable mProcessed;
        std::list<Job> mJobs;
        JobQueue mWaiting;
        std::set<std::tuple<AgentBounds, TilePosition>> mPushed;
        Misc::ScopeGuarded<TilePosition> mPlayerTile;
        NavMeshTilesCache mNavMeshTilesCache;
        Misc::ScopeGuarded<std::set<std::tuple<AgentBounds, TilePosition>>> mProcessingTiles;
        std::map<std::tuple<AgentBounds, TilePosition>, std::chrono::steady_clock::time_point> mLastUpdates;
        std::set<std::tuple<AgentBounds, TilePosition>> mPresentTiles;
        std::vector<std::thread> mThreads;
        std::unique_ptr<DbWorker> mDbWorker;
        std::atomic_size_t mDbGetTileHits{ 0 };

        void process() noexcept;

        JobStatus processJob(Job& job);

        inline JobStatus processInitialJob(Job& job, GuardedNavMeshCacheItem& navMeshCacheItem);

        inline JobStatus processJobWithDbResult(Job& job, GuardedNavMeshCacheItem& navMeshCacheItem);

        inline JobStatus handleUpdateNavMeshStatus(UpdateNavMeshStatus status, const Job& job,
            const GuardedNavMeshCacheItem& navMeshCacheItem, const RecastMesh& recastMesh);

        JobIt getNextJob();

        void postThreadJob(JobIt job, std::deque<JobIt>& queue);

        void writeDebugFiles(const Job& job, const RecastMesh* recastMesh) const;

        bool lockTile(std::size_t jobId, const AgentBounds& agentBounds, const TilePosition& changedTile);

        void unlockTile(std::size_t jobId, const AgentBounds& agentBounds, const TilePosition& changedTile);

        inline std::size_t getTotalJobs() const;

        void cleanupLastUpdates();

        inline void waitUntilJobsDoneForNotPresentTiles(Loading::Listener* listener);

        inline void waitUntilAllJobsDone();
    };
}

#endif
