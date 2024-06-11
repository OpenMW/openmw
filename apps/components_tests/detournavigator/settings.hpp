#ifndef OPENMW_TEST_SUITE_DETOURNAVIGATOR_SETTINGS_H
#define OPENMW_TEST_SUITE_DETOURNAVIGATOR_SETTINGS_H

#include <components/detournavigator/settings.hpp>

#include <chrono>
#include <limits>

namespace DetourNavigator
{
    namespace Tests
    {
        inline Settings makeSettings()
        {
            Settings result;
            result.mEnableWriteRecastMeshToFile = false;
            result.mEnableWriteNavMeshToFile = false;
            result.mEnableRecastMeshFileNameRevision = false;
            result.mEnableNavMeshFileNameRevision = false;
            result.mRecast.mBorderSize = 16;
            result.mRecast.mCellHeight = 0.2f;
            result.mRecast.mCellSize = 0.2f;
            result.mRecast.mDetailSampleDist = 6;
            result.mRecast.mDetailSampleMaxError = 1;
            result.mRecast.mMaxClimb = 34;
            result.mRecast.mMaxSimplificationError = 1.3f;
            result.mRecast.mMaxSlope = 49;
            result.mRecast.mRecastScaleFactor = 0.017647058823529415f;
            result.mRecast.mSwimHeightScale = 0.89999997615814208984375f;
            result.mRecast.mMaxEdgeLen = 12;
            result.mDetour.mMaxNavMeshQueryNodes = 2048;
            result.mRecast.mMaxVertsPerPoly = 6;
            result.mRecast.mRegionMergeArea = 400;
            result.mRecast.mRegionMinArea = 64;
            result.mRecast.mTileSize = 64;
            result.mWaitUntilMinDistanceToPlayer = std::numeric_limits<int>::max();
            result.mAsyncNavMeshUpdaterThreads = 1;
            result.mMaxNavMeshTilesCacheSize = 1024 * 1024;
            result.mDetour.mMaxPolygonPathSize = 1024;
            result.mDetour.mMaxSmoothPathSize = 1024;
            result.mDetour.mMaxPolys = 4096;
            result.mMaxTilesNumber = 1024;
            result.mMinUpdateInterval = std::chrono::milliseconds(50);
            result.mWriteToNavMeshDb = true;
            return result;
        }
    }
}

#endif
