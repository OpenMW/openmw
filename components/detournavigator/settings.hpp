#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGS_H

#include <string>

namespace DetourNavigator
{
    struct Settings
    {
        bool mEnableWriteRecastMeshToFile;
        bool mEnableWriteNavMeshToFile;
        float mCellHeight;
        float mCellSize;
        float mDetailSampleDist;
        float mDetailSampleMaxError;
        float mMaxClimb;
        float mMaxSimplificationError;
        float mMaxSlope;
        float mRecastScaleFactor;
        int mMaxEdgeLen;
        int mMaxNavMeshQueryNodes;
        int mMaxVertsPerPoly;
        int mRegionMergeSize;
        int mRegionMinSize;
        int mTileSize;
        std::size_t mMaxPolygonPathSize;
        std::size_t mMaxSmoothPathSize;
        std::size_t mTrianglesPerChunk;
        std::string mRecastMeshPathPrefix;
        std::string mNavMeshPathPrefix;
    };
}

#endif
