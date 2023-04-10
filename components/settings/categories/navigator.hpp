#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_NAVIGATOR_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_NAVIGATOR_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct NavigatorCategory
    {
        SettingValue<bool> mEnable{ "Navigator", "enable" };
        SettingValue<float> mRecastScaleFactor{ "Navigator", "recast scale factor", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCellHeight{ "Navigator", "cell height", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCellSize{ "Navigator", "cell size", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mDetailSampleDist{ "Navigator", "detail sample dist",
            makeEqualOrMaxSanitizerFloat(0, 0.9) };
        SettingValue<float> mDetailSampleMaxError{ "Navigator", "detail sample max error", makeMaxSanitizerFloat(0) };
        SettingValue<float> mMaxSimplificationError{ "Navigator", "max simplification error",
            makeMaxSanitizerFloat(0) };
        SettingValue<int> mTileSize{ "Navigator", "tile size", makeMaxSanitizerInt(1) };
        SettingValue<int> mBorderSize{ "Navigator", "border size", makeMaxSanitizerInt(0) };
        SettingValue<int> mMaxEdgeLen{ "Navigator", "max edge len", makeMaxSanitizerInt(0) };
        SettingValue<int> mMaxNavMeshQueryNodes{ "Navigator", "max nav mesh query nodes",
            makeClampSanitizerInt(1, 65535) };
        SettingValue<int> mMaxPolygonsPerTile{ "Navigator", "max polygons per tile",
            makeClampSanitizerInt(1, 1 << 21) };
        SettingValue<int> mMaxVertsPerPoly{ "Navigator", "max verts per poly", makeMaxSanitizerInt(3) };
        SettingValue<int> mRegionMergeArea{ "Navigator", "region merge area", makeMaxSanitizerInt(0) };
        SettingValue<int> mRegionMinArea{ "Navigator", "region min area", makeMaxSanitizerInt(0) };
        SettingValue<std::size_t> mAsyncNavMeshUpdaterThreads{ "Navigator", "async nav mesh updater threads",
            makeMaxSanitizerSize(1) };
        SettingValue<std::size_t> mMaxNavMeshTilesCacheSize{ "Navigator", "max nav mesh tiles cache size" };
        SettingValue<std::size_t> mMaxPolygonPathSize{ "Navigator", "max polygon path size" };
        SettingValue<std::size_t> mMaxSmoothPathSize{ "Navigator", "max smooth path size" };
        SettingValue<bool> mEnableWriteRecastMeshToFile{ "Navigator", "enable write recast mesh to file" };
        SettingValue<bool> mEnableWriteNavMeshToFile{ "Navigator", "enable write nav mesh to file" };
        SettingValue<bool> mEnableRecastMeshFileNameRevision{ "Navigator", "enable recast mesh file name revision" };
        SettingValue<bool> mEnableNavMeshFileNameRevision{ "Navigator", "enable nav mesh file name revision" };
        SettingValue<std::string> mRecastMeshPathPrefix{ "Navigator", "recast mesh path prefix" };
        SettingValue<std::string> mNavMeshPathPrefix{ "Navigator", "nav mesh path prefix" };
        SettingValue<bool> mEnableNavMeshRender{ "Navigator", "enable nav mesh render" };
        SettingValue<std::string> mNavMeshRenderMode{ "Navigator", "nav mesh render mode",
            makeEnumSanitizerString({ "area type", "update frequency" }) };
        SettingValue<bool> mEnableAgentsPathsRender{ "Navigator", "enable agents paths render" };
        SettingValue<bool> mEnableRecastMeshRender{ "Navigator", "enable recast mesh render" };
        SettingValue<int> mMaxTilesNumber{ "Navigator", "max tiles number", makeMaxSanitizerInt(0) };
        SettingValue<int> mMinUpdateIntervalMs{ "Navigator", "min update interval ms", makeMaxSanitizerInt(0) };
        SettingValue<int> mWaitUntilMinDistanceToPlayer{ "Navigator", "wait until min distance to player",
            makeMaxSanitizerInt(0) };
        SettingValue<bool> mEnableNavMeshDiskCache{ "Navigator", "enable nav mesh disk cache" };
        SettingValue<bool> mWriteToNavmeshdb{ "Navigator", "write to navmeshdb" };
        SettingValue<std::uint64_t> mMaxNavmeshdbFileSize{ "Navigator", "max navmeshdb file size" };
    };
}

#endif
