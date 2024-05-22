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
    struct JobQueueStats
    {
        std::size_t mRemoving = 0;
        std::size_t mUpdating = 0;
        std::size_t mDelayed = 0;
    };

    struct DbJobQueueStats
    {
        std::size_t mReadingJobs = 0;
        std::size_t mWritingJobs = 0;
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
        JobQueueStats mWaiting;
        std::size_t mPushed = 0;
        std::size_t mProcessing = 0;
        std::size_t mDbGetTileHits = 0;
        std::optional<DbWorkerStats> mDb;
        NavMeshTilesCacheStats mCache;
    };

    struct TileCachedRecastMeshManagerStats
    {
        std::size_t mTiles = 0;
        std::size_t mObjects = 0;
        std::size_t mHeightfields = 0;
        std::size_t mWater = 0;
    };

    struct Stats
    {
        AsyncNavMeshUpdaterStats mUpdater;
        TileCachedRecastMeshManagerStats mRecast;
    };

    void reportStats(const Stats& stats, unsigned int frameNumber, osg::Stats& out);
}

#endif
