#include "settings.hpp"

#include <components/settings/settings.hpp>
#include <components/misc/constants.hpp>

#include <algorithm>

namespace DetourNavigator
{
    RecastSettings makeRecastSettingsFromSettingsManager()
    {
        constexpr float epsilon = std::numeric_limits<float>::epsilon();

        RecastSettings result;

        result.mBorderSize = std::max(0, ::Settings::Manager::getInt("border size", "Navigator"));
        result.mCellHeight = std::max(epsilon, ::Settings::Manager::getFloat("cell height", "Navigator"));
        result.mCellSize = std::max(epsilon, ::Settings::Manager::getFloat("cell size", "Navigator"));
        result.mDetailSampleDist = std::max(0.0f, ::Settings::Manager::getFloat("detail sample dist", "Navigator"));
        result.mDetailSampleMaxError = std::max(0.0f, ::Settings::Manager::getFloat("detail sample max error", "Navigator"));
        result.mMaxClimb = Constants::sStepSizeUp;
        result.mMaxSimplificationError = std::max(0.0f, ::Settings::Manager::getFloat("max simplification error", "Navigator"));
        result.mMaxSlope = Constants::sMaxSlope;
        result.mRecastScaleFactor = std::max(epsilon, ::Settings::Manager::getFloat("recast scale factor", "Navigator"));
        result.mSwimHeightScale = 0;
        result.mMaxEdgeLen = std::max(0, ::Settings::Manager::getInt("max edge len", "Navigator"));
        result.mMaxVertsPerPoly = std::max(3, ::Settings::Manager::getInt("max verts per poly", "Navigator"));
        result.mRegionMergeArea = std::max(0, ::Settings::Manager::getInt("region merge area", "Navigator"));
        result.mRegionMinArea = std::max(0, ::Settings::Manager::getInt("region min area", "Navigator"));
        result.mTileSize = std::max(1, ::Settings::Manager::getInt("tile size", "Navigator"));

        return result;
    }

    DetourSettings makeDetourSettingsFromSettingsManager()
    {
        DetourSettings result;

        result.mMaxNavMeshQueryNodes = std::clamp(::Settings::Manager::getInt("max nav mesh query nodes", "Navigator"), 1, 65535);
        result.mMaxPolys = std::clamp(::Settings::Manager::getInt("max polygons per tile", "Navigator"), 1, (1 << 22) - 1);
        result.mMaxPolygonPathSize = static_cast<std::size_t>(std::max(0, ::Settings::Manager::getInt("max polygon path size", "Navigator")));
        result.mMaxSmoothPathSize = static_cast<std::size_t>(std::max(0, ::Settings::Manager::getInt("max smooth path size", "Navigator")));

        return result;
    }

    Settings makeSettingsFromSettingsManager()
    {
        Settings result;

        result.mRecast = makeRecastSettingsFromSettingsManager();
        result.mDetour = makeDetourSettingsFromSettingsManager();
        result.mMaxTilesNumber = std::max(0, ::Settings::Manager::getInt("max tiles number", "Navigator"));
        result.mWaitUntilMinDistanceToPlayer = ::Settings::Manager::getInt("wait until min distance to player", "Navigator");
        result.mAsyncNavMeshUpdaterThreads = static_cast<std::size_t>(std::max(0, ::Settings::Manager::getInt("async nav mesh updater threads", "Navigator")));
        result.mMaxNavMeshTilesCacheSize = static_cast<std::size_t>(std::max(std::int64_t {0}, ::Settings::Manager::getInt64("max nav mesh tiles cache size", "Navigator")));
        result.mEnableWriteRecastMeshToFile = ::Settings::Manager::getBool("enable write recast mesh to file", "Navigator");
        result.mEnableWriteNavMeshToFile = ::Settings::Manager::getBool("enable write nav mesh to file", "Navigator");
        result.mRecastMeshPathPrefix = ::Settings::Manager::getString("recast mesh path prefix", "Navigator");
        result.mNavMeshPathPrefix = ::Settings::Manager::getString("nav mesh path prefix", "Navigator");
        result.mEnableRecastMeshFileNameRevision = ::Settings::Manager::getBool("enable recast mesh file name revision", "Navigator");
        result.mEnableNavMeshFileNameRevision = ::Settings::Manager::getBool("enable nav mesh file name revision", "Navigator");
        result.mMinUpdateInterval = std::chrono::milliseconds(::Settings::Manager::getInt("min update interval ms", "Navigator"));
        result.mNavMeshVersion = ::Settings::Manager::getInt("nav mesh version", "Navigator");
        result.mEnableNavMeshDiskCache = ::Settings::Manager::getBool("enable nav mesh disk cache", "Navigator");
        result.mWriteToNavMeshDb = ::Settings::Manager::getBool("write to navmeshdb", "Navigator");
        result.mMaxDbFileSize = static_cast<std::uint64_t>(::Settings::Manager::getInt64("max navmeshdb file size", "Navigator"));

        return result;
    }
}
