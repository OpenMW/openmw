#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_NAVIGATOR_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_NAVIGATOR_H

#include <components/settings/navmeshrendermode.hpp>
#include <components/settings/sanitizerimpl.hpp>
#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct NavigatorCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mEnable{ mIndex, "Navigator", "enable" };
        SettingValue<float> mRecastScaleFactor{ mIndex, "Navigator", "recast scale factor",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCellHeight{ mIndex, "Navigator", "cell height", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCellSize{ mIndex, "Navigator", "cell size", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mDetailSampleDist{ mIndex, "Navigator", "detail sample dist",
            makeEqualOrMaxSanitizerFloat(0, 0.9f) };
        SettingValue<float> mDetailSampleMaxError{ mIndex, "Navigator", "detail sample max error",
            makeMaxSanitizerFloat(0) };
        SettingValue<float> mMaxSimplificationError{ mIndex, "Navigator", "max simplification error",
            makeMaxSanitizerFloat(0) };
        SettingValue<int> mTileSize{ mIndex, "Navigator", "tile size", makeMaxSanitizerInt(1) };
        SettingValue<int> mBorderSize{ mIndex, "Navigator", "border size", makeMaxSanitizerInt(0) };
        SettingValue<int> mMaxEdgeLen{ mIndex, "Navigator", "max edge len", makeMaxSanitizerInt(0) };
        SettingValue<int> mMaxNavMeshQueryNodes{ mIndex, "Navigator", "max nav mesh query nodes",
            makeClampSanitizerInt(1, 65535) };
        SettingValue<int> mMaxPolygonsPerTile{ mIndex, "Navigator", "max polygons per tile",
            makeClampSanitizerInt(1, 1 << 21) };
        SettingValue<int> mMaxVertsPerPoly{ mIndex, "Navigator", "max verts per poly", makeMaxSanitizerInt(3) };
        SettingValue<int> mRegionMergeArea{ mIndex, "Navigator", "region merge area", makeMaxSanitizerInt(0) };
        SettingValue<int> mRegionMinArea{ mIndex, "Navigator", "region min area", makeMaxSanitizerInt(0) };
        SettingValue<std::size_t> mAsyncNavMeshUpdaterThreads{ mIndex, "Navigator", "async nav mesh updater threads",
            makeMaxSanitizerSize(1) };
        SettingValue<std::size_t> mMaxNavMeshTilesCacheSize{ mIndex, "Navigator", "max nav mesh tiles cache size" };
        SettingValue<std::size_t> mMaxPolygonPathSize{ mIndex, "Navigator", "max polygon path size" };
        SettingValue<std::size_t> mMaxSmoothPathSize{ mIndex, "Navigator", "max smooth path size" };
        SettingValue<bool> mEnableWriteRecastMeshToFile{ mIndex, "Navigator", "enable write recast mesh to file" };
        SettingValue<bool> mEnableWriteNavMeshToFile{ mIndex, "Navigator", "enable write nav mesh to file" };
        SettingValue<bool> mEnableRecastMeshFileNameRevision{ mIndex, "Navigator",
            "enable recast mesh file name revision" };
        SettingValue<bool> mEnableNavMeshFileNameRevision{ mIndex, "Navigator", "enable nav mesh file name revision" };
        SettingValue<std::string> mRecastMeshPathPrefix{ mIndex, "Navigator", "recast mesh path prefix" };
        SettingValue<std::string> mNavMeshPathPrefix{ mIndex, "Navigator", "nav mesh path prefix" };
        SettingValue<bool> mEnableNavMeshRender{ mIndex, "Navigator", "enable nav mesh render" };
        SettingValue<NavMeshRenderMode> mNavMeshRenderMode{ mIndex, "Navigator", "nav mesh render mode" };
        SettingValue<bool> mEnableAgentsPathsRender{ mIndex, "Navigator", "enable agents paths render" };
        SettingValue<bool> mEnableRecastMeshRender{ mIndex, "Navigator", "enable recast mesh render" };
        SettingValue<int> mMaxTilesNumber{ mIndex, "Navigator", "max tiles number", makeMaxSanitizerInt(0) };
        SettingValue<int> mMinUpdateIntervalMs{ mIndex, "Navigator", "min update interval ms", makeMaxSanitizerInt(0) };
        SettingValue<int> mWaitUntilMinDistanceToPlayer{ mIndex, "Navigator", "wait until min distance to player",
            makeMaxSanitizerInt(0) };
        SettingValue<bool> mEnableNavMeshDiskCache{ mIndex, "Navigator", "enable nav mesh disk cache" };
        SettingValue<bool> mWriteToNavmeshdb{ mIndex, "Navigator", "write to navmeshdb" };
        SettingValue<std::uint64_t> mMaxNavmeshdbFileSize{ mIndex, "Navigator", "max navmeshdb file size" };
        SettingValue<bool> mWaitForAllJobsOnExit{ mIndex, "Navigator", "wait for all jobs on exit" };
    };
}

#endif
