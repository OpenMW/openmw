#include "stats.hpp"

#include <osg/Stats>

namespace DetourNavigator
{
    namespace
    {
        void reportStats(const AsyncNavMeshUpdaterStats& stats, unsigned int frameNumber, osg::Stats& out)
        {
            out.setAttribute(frameNumber, "NavMesh Jobs", static_cast<double>(stats.mJobs));
            out.setAttribute(frameNumber, "NavMesh Waiting", static_cast<double>(stats.mWaiting));
            out.setAttribute(frameNumber, "NavMesh Pushed", static_cast<double>(stats.mPushed));
            out.setAttribute(frameNumber, "NavMesh Processing", static_cast<double>(stats.mProcessing));

            if (stats.mDb.has_value())
            {
                out.setAttribute(frameNumber, "NavMesh DbJobs Write", static_cast<double>(stats.mDb->mJobs.mWritingJobs));
                out.setAttribute(frameNumber, "NavMesh DbJobs Read", static_cast<double>(stats.mDb->mJobs.mReadingJobs));

                if (stats.mDb->mGetTileCount > 0)
                    out.setAttribute(frameNumber, "NavMesh DbCacheHitRate", static_cast<double>(stats.mDbGetTileHits)
                                     / static_cast<double>(stats.mDb->mGetTileCount) * 100.0);
            }

            out.setAttribute(frameNumber, "NavMesh CacheSize", static_cast<double>(stats.mCache.mNavMeshCacheSize));
            out.setAttribute(frameNumber, "NavMesh UsedTiles", static_cast<double>(stats.mCache.mUsedNavMeshTiles));
            out.setAttribute(frameNumber, "NavMesh CachedTiles", static_cast<double>(stats.mCache.mCachedNavMeshTiles));
            if (stats.mCache.mGetCount > 0)
                out.setAttribute(frameNumber, "NavMesh CacheHitRate", static_cast<double>(stats.mCache.mHitCount)
                                 / stats.mCache.mGetCount * 100.0);
        }
    }

    void reportStats(const Stats& stats, unsigned int frameNumber, osg::Stats& out)
    {
        if (stats.mUpdater.has_value())
            reportStats(*stats.mUpdater, frameNumber, out);
    }
}
