#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H

#include <boost/optional.hpp>

#include <string>

namespace DetourNavigator
{
    struct Settings
    {
        bool mEnableWriteRecastMeshToFile = false;
        bool mEnableWriteNavMeshToFile = false;
        bool mEnableRecastMeshFileNameRevision = false;
        bool mEnableNavMeshFileNameRevision = false;
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
        int mMaxNavMeshQueryNodes = 0;
        int mMaxPolys = 0;
        int mMaxTilesNumber = 0;
        int mMaxVertsPerPoly = 0;
        int mRegionMergeSize = 0;
        int mRegionMinSize = 0;
        int mTileSize = 0;
        std::size_t mAsyncNavMeshUpdaterThreads = 0;
        std::size_t mMaxNavMeshTilesCacheSize = 0;
        std::size_t mMaxPolygonPathSize = 0;
        std::size_t mMaxSmoothPathSize = 0;
        std::size_t mTrianglesPerChunk = 0;
        std::string mRecastMeshPathPrefix;
        std::string mNavMeshPathPrefix;
    };

    boost::optional<Settings> makeSettingsFromSettingsManager();
}

#endif
