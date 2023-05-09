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
        const std::string mWorldspace;
        const TilePosition mChangedTile;
        const std::chrono::steady_clock::time_point mProcessTime;
        unsigned mTryNumber = 0;
        ChangeType mChangeType;
        int mDistanceToPlayer;
        const int mDistanceToOrigin;
        JobState mState = JobState::Initial;
        std::vector<std::byte> mInput;
        std::shared_ptr<RecastMesh> mRecastMesh;
        std::optional<TileData> mCachedTileData;
        std::unique_ptr<PreparedNavMeshData> mGeneratedNavMeshData;

        Job(const AgentBounds& agentBounds, std::weak_ptr<GuardedNavMeshCacheItem> navMeshCacheItem,
            std::string_view worldspace, const TilePosition& changedTile, ChangeType changeType, int distanceToPlayer,
            std::chrono::steady_clock::time_point processTime);
    };

    using JobIt = std::list<Job>::iterator;

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

        void update(TilePosition playerTile, int maxTiles);

        void stop();

        DbJobQueueStats getStats() const;

    private:
        mutable std::mutex mMutex;
        std::condition_variable mHasJob;
        std::deque<JobIt> mJobs;
        bool mShouldStop = false;
        std::size_t mWritingJobs = 0;
        std::size_t mReadingJobs = 0;
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

        void updateJobs(TilePosition playerTile, int maxTiles) { mQueue.update(playerTile, maxTiles); }

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
            const TilePosition& playerTile, std::string_view worldspace,
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
        std::deque<JobIt> mWaiting;
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

        void repost(JobIt job);

        bool lockTile(const AgentBounds& agentBounds, const TilePosition& changedTile);

        void unlockTile(const AgentBounds& agentBounds, const TilePosition& changedTile);

        inline std::size_t getTotalJobs() const;

        void cleanupLastUpdates();

        inline void waitUntilJobsDoneForNotPresentTiles(Loading::Listener* listener);

        inline void waitUntilAllJobsDone();
    };
}

#endif
