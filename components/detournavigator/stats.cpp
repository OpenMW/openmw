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
                out.setAttribute(
                    frameNumber, "NavMesh DbJobs Write", static_cast<double>(stats.mDb->mJobs.mWritingJobs));
                out.setAttribute(
                    frameNumber, "NavMesh DbJobs Read", static_cast<double>(stats.mDb->mJobs.mReadingJobs));

                out.setAttribute(frameNumber, "NavMesh DbCache Get", static_cast<double>(stats.mDb->mGetTileCount));
                out.setAttribute(frameNumber, "NavMesh DbCache Hit", static_cast<double>(stats.mDbGetTileHits));
            }

            out.setAttribute(frameNumber, "NavMesh CacheSize", static_cast<double>(stats.mCache.mNavMeshCacheSize));
            out.setAttribute(frameNumber, "NavMesh UsedTiles", static_cast<double>(stats.mCache.mUsedNavMeshTiles));
            out.setAttribute(frameNumber, "NavMesh CachedTiles", static_cast<double>(stats.mCache.mCachedNavMeshTiles));
            out.setAttribute(frameNumber, "NavMesh Cache Get", static_cast<double>(stats.mCache.mGetCount));
            out.setAttribute(frameNumber, "NavMesh Cache Hit", static_cast<double>(stats.mCache.mHitCount));
        }
    }

    void reportStats(const Stats& stats, unsigned int frameNumber, osg::Stats& out)
    {
        if (stats.mUpdater.has_value())
            reportStats(*stats.mUpdater, frameNumber, out);
    }
}
