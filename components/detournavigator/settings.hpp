#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H

#include <string>

namespace DetourNavigator
{
    struct Settings
    {
        bool mEnableWriteRecastMeshToFile;
        bool mEnableWriteNavMeshToFile;
        bool mEnableRecastMeshFileNameRevision;
        bool mEnableNavMeshFileNameRevision;
        float mCellHeight;
        float mCellSize;
        float mDetailSampleDist;
        float mDetailSampleMaxError;
        float mMaxClimb;
        float mMaxSimplificationError;
        float mMaxSlope;
        float mRecastScaleFactor;
        float mSwimHeightScale;
        int mBorderSize;
        int mMaxEdgeLen;
        int mMaxNavMeshQueryNodes;
        int mMaxPolys;
        int mMaxVertsPerPoly;
        int mRegionMergeSize;
        int mRegionMinSize;
        int mTileSize;
        std::size_t mAsyncNavMeshUpdaterThreads;
        std::size_t mMaxNavMeshTilesCacheSize;
        std::size_t mMaxPolygonPathSize;
        std::size_t mMaxSmoothPathSize;
        std::size_t mTrianglesPerChunk;
        std::string mRecastMeshPathPrefix;
        std::string mNavMeshPathPrefix;
    };
}

#endif
