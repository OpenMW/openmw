#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H

#include <chrono>
#include <string>

namespace DetourNavigator
{
    struct RecastSettings
    {
        float mCellHeight = 0;
        float mCellSize = 0;
        float mDetailSampleDist = 0;
        float mDetailSampleMaxError = 0;
        float mMaxClimb = 0;
        float mMaxSimplificationError = 0;
        float mMaxSlope = 0;
        float mRecastScaleFactor = 0;
        float mSwimHeightScale = 0;
        int mBorderSize = 0;
        int mMaxEdgeLen = 0;
        int mMaxVertsPerPoly = 0;
        int mRegionMergeArea = 0;
        int mRegionMinArea = 0;
        int mTileSize = 0;
    };

    struct DetourSettings
    {
        int mMaxPolys = 0;
        int mMaxNavMeshQueryNodes = 0;
        std::size_t mMaxPolygonPathSize = 0;
        std::size_t mMaxSmoothPathSize = 0;
    };

    struct Settings
    {
        bool mEnableWriteRecastMeshToFile = false;
        bool mEnableWriteNavMeshToFile = false;
        bool mEnableRecastMeshFileNameRevision = false;
        bool mEnableNavMeshFileNameRevision = false;
        bool mEnableNavMeshDiskCache = false;
        bool mWriteToNavMeshDb = false;
        RecastSettings mRecast;
        DetourSettings mDetour;
        int mWaitUntilMinDistanceToPlayer = 0;
        int mMaxTilesNumber = 0;
        std::size_t mAsyncNavMeshUpdaterThreads = 0;
        std::size_t mMaxNavMeshTilesCacheSize = 0;
        std::string mRecastMeshPathPrefix;
        std::string mNavMeshPathPrefix;
        std::chrono::milliseconds mMinUpdateInterval;
        std::uint64_t mMaxDbFileSize = 0;
    };

    inline constexpr std::int64_t navMeshFormatVersion = 2;

    Settings makeSettingsFromSettingsManager();
}

#endif
