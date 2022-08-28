#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_STATS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_STATS_H

#include <cstddef>
#include <optional>

namespace osg
{
    class Stats;
}

namespace DetourNavigator
{
    struct DbJobQueueStats
    {
        std::size_t mWritingJobs = 0;
        std::size_t mReadingJobs = 0;
    };

    struct DbWorkerStats
    {
        DbJobQueueStats mJobs;
        std::size_t mGetTileCount = 0;
    };

    struct NavMeshTilesCacheStats
    {
        std::size_t mNavMeshCacheSize = 0;
        std::size_t mUsedNavMeshTiles = 0;
        std::size_t mCachedNavMeshTiles = 0;
        std::size_t mHitCount = 0;
        std::size_t mGetCount = 0;
    };

    struct AsyncNavMeshUpdaterStats
    {
        std::size_t mJobs = 0;
        std::size_t mWaiting = 0;
        std::size_t mPushed = 0;
        std::size_t mProcessing = 0;
        std::size_t mDbGetTileHits = 0;
        std::optional<DbWorkerStats> mDb;
        NavMeshTilesCacheStats mCache;
    };

    struct Stats
    {
        std::optional<AsyncNavMeshUpdaterStats> mUpdater;
    };

    void reportStats(const Stats& stats, unsigned int frameNumber, osg::Stats& out);
}

#endif
