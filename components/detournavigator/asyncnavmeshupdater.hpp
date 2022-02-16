#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_ASYNCNAVMESHUPDATER_H

#include "navmeshcacheitem.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "tilecachedrecastmeshmanager.hpp"
#include "tileposition.hpp"
#include "navmeshtilescache.hpp"
#include "waitconditiontype.hpp"
#include "navmeshdb.hpp"
#include "changetype.hpp"

#include <osg/Vec3f>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <deque>
#include <set>
#include <thread>
#include <tuple>
#include <list>
#include <optional>

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
        const osg::Vec3f mAgentHalfExtents;
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

        Job(const osg::Vec3f& agentHalfExtents, std::weak_ptr<GuardedNavMeshCacheItem> navMeshCacheItem,
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

    inline std::ostream& operator<<(std::ostream& stream, JobStatus value)
    {
        switch (value)
        {
            case JobStatus::Done: return stream << "JobStatus::Done";
            case JobStatus::Fail: return stream << "JobStatus::Fail";
            case JobStatus::MemoryCacheMiss: return stream << "JobStatus::MemoryCacheMiss";
        }
        return stream << "JobStatus::" << static_cast<std::underlying_type_t<JobState>>(value);
    }

    class DbJobQueue
    {
    public:
        void push(JobIt job);

        std::optional<JobIt> pop();

        void update(TilePosition playerTile, int maxTiles);

        void stop();

        std::size_t size() const;

    private:
        mutable std::mutex mMutex;
        std::condition_variable mHasJob;
        std::deque<JobIt> mJobs;
        bool mShouldStop = false;
    };

    class AsyncNavMeshUpdater;

    class DbWorker
    {
    public:
        struct Stats
        {
            std::size_t mJobs = 0;
            std::size_t mGetTileCount = 0;
        };

        DbWorker(AsyncNavMeshUpdater& updater, std::unique_ptr<NavMeshDb>&& db,
            TileVersion version, const RecastSettings& recastSettings, bool writeToDb);

        ~DbWorker();

        Stats getStats() const;

        void enqueueJob(JobIt job);

        void updateJobs(TilePosition playerTile, int maxTiles) { mQueue.update(playerTile, maxTiles); }

        void stop();

    private:
        AsyncNavMeshUpdater& mUpdater;
        const RecastSettings& mRecastSettings;
        const std::unique_ptr<NavMeshDb> mDb;
        const TileVersion mVersion;
        const bool mWriteToDb;
        TileId mNextTileId;
        ShapeId mNextShapeId;
        DbJobQueue mQueue;
        std::atomic_bool mShouldStop {false};
        std::atomic_size_t mGetTileCount {0};
        std::size_t mWrites = 0;
        std::thread mThread;

        inline void run() noexcept;

        inline void processJob(JobIt job);

        inline void processReadingJob(JobIt job);

        inline void processWritingJob(JobIt job);
    };

    class AsyncNavMeshUpdater
    {
    public:
        struct Stats
        {
            std::size_t mJobs = 0;
            std::size_t mWaiting = 0;
            std::size_t mPushed = 0;
            std::size_t mProcessing = 0;
            std::size_t mDbGetTileHits = 0;
            std::optional<DbWorker::Stats> mDb;
            NavMeshTilesCache::Stats mCache;
        };

        AsyncNavMeshUpdater(const Settings& settings, TileCachedRecastMeshManager& recastMeshManager,
            OffMeshConnectionsManager& offMeshConnectionsManager, std::unique_ptr<NavMeshDb>&& db);
        ~AsyncNavMeshUpdater();

        void post(const osg::Vec3f& agentHalfExtents, const SharedNavMeshCacheItem& navMeshCacheItem,
            const TilePosition& playerTile, std::string_view worldspace,
            const std::map<TilePosition, ChangeType>& changedTiles);

        void wait(Loading::Listener& listener, WaitConditionType waitConditionType);

        Stats getStats() const;

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
        std::set<std::tuple<osg::Vec3f, TilePosition>> mPushed;
        Misc::ScopeGuarded<TilePosition> mPlayerTile;
        NavMeshTilesCache mNavMeshTilesCache;
        Misc::ScopeGuarded<std::set<std::tuple<osg::Vec3f, TilePosition>>> mProcessingTiles;
        std::map<std::tuple<osg::Vec3f, TilePosition>, std::chrono::steady_clock::time_point> mLastUpdates;
        std::set<std::tuple<osg::Vec3f, TilePosition>> mPresentTiles;
        std::vector<std::thread> mThreads;
        std::unique_ptr<DbWorker> mDbWorker;
        std::atomic_size_t mDbGetTileHits {0};

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

        bool lockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile);

        void unlockTile(const osg::Vec3f& agentHalfExtents, const TilePosition& changedTile);

        inline std::size_t getTotalJobs() const;

        void cleanupLastUpdates();

        int waitUntilJobsDoneForNotPresentTiles(const std::size_t initialJobsLeft, std::size_t& maxJobsLeft, Loading::Listener& listener);

        void waitUntilAllJobsDone();
    };

    void reportStats(const AsyncNavMeshUpdater::Stats& stats, unsigned int frameNumber, osg::Stats& out);
}

#endif
