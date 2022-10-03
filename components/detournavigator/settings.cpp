#include "settings.hpp"

#include <components/misc/constants.hpp>
#include <components/settings/values.hpp>

namespace DetourNavigator
{
    RecastSettings makeRecastSettingsFromSettingsManager()
    {
        RecastSettings result;

        result.mBorderSize = ::Settings::navigator().mBorderSize;
        result.mCellHeight = ::Settings::navigator().mCellHeight;
        result.mCellSize = ::Settings::navigator().mCellSize;
        result.mDetailSampleDist = ::Settings::navigator().mDetailSampleDist;
        result.mDetailSampleMaxError = ::Settings::navigator().mDetailSampleMaxError;
        result.mMaxClimb = Constants::sStepSizeUp;
        result.mMaxSimplificationError = ::Settings::navigator().mMaxSimplificationError;
        result.mMaxSlope = Constants::sMaxSlope;
        result.mRecastScaleFactor = ::Settings::navigator().mRecastScaleFactor;
        result.mSwimHeightScale = 0;
        result.mMaxEdgeLen = ::Settings::navigator().mMaxEdgeLen;
        result.mMaxVertsPerPoly = ::Settings::navigator().mMaxVertsPerPoly;
        result.mRegionMergeArea = ::Settings::navigator().mRegionMergeArea;
        result.mRegionMinArea = ::Settings::navigator().mRegionMinArea;
        result.mTileSize = ::Settings::navigator().mTileSize;

        return result;
    }

    DetourSettings makeDetourSettingsFromSettingsManager()
    {
        DetourSettings result;

        result.mMaxNavMeshQueryNodes = ::Settings::navigator().mMaxNavMeshQueryNodes;
        result.mMaxPolys = ::Settings::navigator().mMaxPolygonsPerTile;
        result.mMaxPolygonPathSize = ::Settings::navigator().mMaxPolygonPathSize;
        result.mMaxSmoothPathSize = ::Settings::navigator().mMaxSmoothPathSize;

        return result;
    }

    Settings makeSettingsFromSettingsManager()
    {
        Settings result;

        result.mRecast = makeRecastSettingsFromSettingsManager();
        result.mDetour = makeDetourSettingsFromSettingsManager();
        result.mMaxTilesNumber = ::Settings::navigator().mMaxTilesNumber;
        result.mWaitUntilMinDistanceToPlayer = ::Settings::navigator().mWaitUntilMinDistanceToPlayer;
        result.mAsyncNavMeshUpdaterThreads = ::Settings::navigator().mAsyncNavMeshUpdaterThreads;
        result.mMaxNavMeshTilesCacheSize = ::Settings::navigator().mMaxNavMeshTilesCacheSize;
        result.mEnableWriteRecastMeshToFile = ::Settings::navigator().mEnableWriteRecastMeshToFile;
        result.mEnableWriteNavMeshToFile = ::Settings::navigator().mEnableWriteNavMeshToFile;
        result.mRecastMeshPathPrefix = ::Settings::navigator().mRecastMeshPathPrefix;
        result.mNavMeshPathPrefix = ::Settings::navigator().mNavMeshPathPrefix;
        result.mEnableRecastMeshFileNameRevision = ::Settings::navigator().mEnableRecastMeshFileNameRevision;
        result.mEnableNavMeshFileNameRevision = ::Settings::navigator().mEnableNavMeshFileNameRevision;
        result.mMinUpdateInterval = std::chrono::milliseconds(::Settings::navigator().mMinUpdateIntervalMs);
        result.mEnableNavMeshDiskCache = ::Settings::navigator().mEnableNavMeshDiskCache;
        result.mWriteToNavMeshDb = ::Settings::navigator().mWriteToNavmeshdb;
        result.mMaxDbFileSize = ::Settings::navigator().mMaxNavmeshdbFileSize;

        return result;
    }
}
