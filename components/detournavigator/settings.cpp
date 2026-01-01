#include "settings.hpp"

#include <components/misc/constants.hpp>
#include <components/settings/values.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace DetourNavigator
{
    namespace
    {
        struct NavMeshLimits
        {
            int mMaxTiles;
            int mMaxPolys;
        };

        template <class T>
        unsigned long getMinValuableBitsNumber(const T value)
        {
            unsigned long power = 0;
            while (power < sizeof(T) * 8 && (static_cast<T>(1) << power) < value)
                ++power;
            return power;
        }

        NavMeshLimits getNavMeshTileLimits(const DetourSettings& settings)
        {
            // Max tiles and max polys affect how the tile IDs are caculated.
            // There are 22 bits available for identifying a tile and a polygon.
            constexpr int polysAndTilesBits = 22;
            const unsigned long polysBits = getMinValuableBitsNumber(settings.mMaxPolys);

            if (polysBits >= polysAndTilesBits)
                throw std::invalid_argument("Too many polygons per tile: " + std::to_string(settings.mMaxPolys));

            const unsigned long tilesBits = polysAndTilesBits - polysBits;

            return NavMeshLimits{
                .mMaxTiles = static_cast<int>(1 << tilesBits),
                .mMaxPolys = static_cast<int>(1 << polysBits),
            };
        }

        RecastSettings makeRecastSettingsFromSettingsManager(Debug::Level maxLogLevel)
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
            result.mMaxLogLevel = maxLogLevel;

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
    }

    Settings makeSettingsFromSettingsManager(Debug::Level maxLogLevel)
    {
        Settings result;

        result.mRecast = makeRecastSettingsFromSettingsManager(maxLogLevel);
        result.mDetour = makeDetourSettingsFromSettingsManager();

        const NavMeshLimits limits = getNavMeshTileLimits(result.mDetour);

        result.mDetour.mMaxPolys = limits.mMaxPolys;

        result.mMaxTilesNumber = std::min(limits.mMaxTiles, ::Settings::navigator().mMaxTilesNumber.get());
        result.mWaitUntilMinDistanceToPlayer = ::Settings::navigator().mWaitUntilMinDistanceToPlayer;
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
        
        // Force disable navmesh generation worker threads regardless of config file settings
        result.mAsyncNavMeshUpdaterThreads = 0;

        return result;
    }
}
