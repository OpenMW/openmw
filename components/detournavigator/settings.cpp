#include "settings.hpp"

#include <components/settings/settings.hpp>

namespace DetourNavigator
{
    boost::optional<Settings> makeSettingsFromSettingsManager()
    {
        if (!::Settings::Manager::getBool("enable", "Navigator"))
            return boost::optional<Settings>();

        Settings navigatorSettings;

        navigatorSettings.mBorderSize = ::Settings::Manager::getInt("border size", "Navigator");
        navigatorSettings.mCellHeight = ::Settings::Manager::getFloat("cell height", "Navigator");
        navigatorSettings.mCellSize = ::Settings::Manager::getFloat("cell size", "Navigator");
        navigatorSettings.mDetailSampleDist = ::Settings::Manager::getFloat("detail sample dist", "Navigator");
        navigatorSettings.mDetailSampleMaxError = ::Settings::Manager::getFloat("detail sample max error", "Navigator");
        navigatorSettings.mMaxClimb = 0;
        navigatorSettings.mMaxSimplificationError = ::Settings::Manager::getFloat("max simplification error", "Navigator");
        navigatorSettings.mMaxSlope = 0;
        navigatorSettings.mRecastScaleFactor = ::Settings::Manager::getFloat("recast scale factor", "Navigator");
        navigatorSettings.mSwimHeightScale = 0;
        navigatorSettings.mMaxEdgeLen = ::Settings::Manager::getInt("max edge len", "Navigator");
        navigatorSettings.mMaxNavMeshQueryNodes = ::Settings::Manager::getInt("max nav mesh query nodes", "Navigator");
        navigatorSettings.mMaxPolys = ::Settings::Manager::getInt("max polygons per tile", "Navigator");
        navigatorSettings.mMaxTilesNumber = ::Settings::Manager::getInt("max tiles number", "Navigator");
        navigatorSettings.mMaxVertsPerPoly = ::Settings::Manager::getInt("max verts per poly", "Navigator");
        navigatorSettings.mRegionMergeSize = ::Settings::Manager::getInt("region merge size", "Navigator");
        navigatorSettings.mRegionMinSize = ::Settings::Manager::getInt("region min size", "Navigator");
        navigatorSettings.mTileSize = ::Settings::Manager::getInt("tile size", "Navigator");
        navigatorSettings.mAsyncNavMeshUpdaterThreads = static_cast<std::size_t>(::Settings::Manager::getInt("async nav mesh updater threads", "Navigator"));
        navigatorSettings.mMaxNavMeshTilesCacheSize = static_cast<std::size_t>(::Settings::Manager::getInt("max nav mesh tiles cache size", "Navigator"));
        navigatorSettings.mMaxPolygonPathSize = static_cast<std::size_t>(::Settings::Manager::getInt("max polygon path size", "Navigator"));
        navigatorSettings.mMaxSmoothPathSize = static_cast<std::size_t>(::Settings::Manager::getInt("max smooth path size", "Navigator"));
        navigatorSettings.mTrianglesPerChunk = static_cast<std::size_t>(::Settings::Manager::getInt("triangles per chunk", "Navigator"));
        navigatorSettings.mEnableWriteRecastMeshToFile = ::Settings::Manager::getBool("enable write recast mesh to file", "Navigator");
        navigatorSettings.mEnableWriteNavMeshToFile = ::Settings::Manager::getBool("enable write nav mesh to file", "Navigator");
        navigatorSettings.mRecastMeshPathPrefix = ::Settings::Manager::getString("recast mesh path prefix", "Navigator");
        navigatorSettings.mNavMeshPathPrefix = ::Settings::Manager::getString("nav mesh path prefix", "Navigator");
        navigatorSettings.mEnableRecastMeshFileNameRevision = ::Settings::Manager::getBool("enable recast mesh file name revision", "Navigator");
        navigatorSettings.mEnableNavMeshFileNameRevision = ::Settings::Manager::getBool("enable nav mesh file name revision", "Navigator");

        return navigatorSettings;
    }
}
