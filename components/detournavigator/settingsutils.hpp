#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGSUTILS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGSUTILS_H

#include "settings.hpp"
#include "tilebounds.hpp"
#include "tileposition.hpp"

#include <osg/Vec2f>
#include <osg/Vec3f>

#include <algorithm>
#include <cmath>

namespace DetourNavigator
{
    inline float toNavMeshCoordinates(const RecastSettings& settings, float value)
    {
        return value * settings.mRecastScaleFactor;
    }

    inline osg::Vec2f toNavMeshCoordinates(const RecastSettings& settings, osg::Vec2f position)
    {
        return position * settings.mRecastScaleFactor;
    }

    inline osg::Vec3f toNavMeshCoordinates(const RecastSettings& settings, osg::Vec3f position)
    {
        std::swap(position.y(), position.z());
        return position * settings.mRecastScaleFactor;
    }

    inline TileBounds toNavMeshCoordinates(const RecastSettings& settings, const TileBounds& value)
    {
        return TileBounds{ toNavMeshCoordinates(settings, value.mMin), toNavMeshCoordinates(settings, value.mMax) };
    }

    inline float fromNavMeshCoordinates(const RecastSettings& settings, float value)
    {
        return value / settings.mRecastScaleFactor;
    }

    inline osg::Vec3f fromNavMeshCoordinates(const RecastSettings& settings, osg::Vec3f position)
    {
        const auto factor = 1.0f / settings.mRecastScaleFactor;
        position *= factor;
        std::swap(position.y(), position.z());
        return position;
    }

    // Returns value in NavMesh coordinates
    inline float getTileSize(const RecastSettings& settings)
    {
        return static_cast<float>(settings.mTileSize) * settings.mCellSize;
    }

    inline int getTilePosition(const RecastSettings& settings, float position)
    {
        const float v = std::floor(position / getTileSize(settings));
        if (v < static_cast<float>(std::numeric_limits<int>::min()))
            return std::numeric_limits<int>::min();
        if (v > static_cast<float>(std::numeric_limits<int>::max() - 1))
            return std::numeric_limits<int>::max() - 1;
        return static_cast<int>(v);
    }

    // Returns integer tile position for position in navmesh coordinates
    inline TilePosition getTilePosition(const RecastSettings& settings, const osg::Vec2f& position)
    {
        return TilePosition(getTilePosition(settings, position.x()), getTilePosition(settings, position.y()));
    }

    // Returns integer tile position for position in navmesh coordinates
    inline TilePosition getTilePosition(const RecastSettings& settings, const osg::Vec3f& position)
    {
        return getTilePosition(settings, osg::Vec2f(position.x(), position.z()));
    }

    // Returns tile bounds in navmesh coordinates
    inline TileBounds makeTileBounds(const RecastSettings& settings, const TilePosition& tilePosition)
    {
        return TileBounds{
            osg::Vec2f(static_cast<float>(tilePosition.x()), static_cast<float>(tilePosition.y()))
                * getTileSize(settings),
            osg::Vec2f(static_cast<float>(tilePosition.x() + 1), static_cast<float>(tilePosition.y() + 1))
                * getTileSize(settings),
        };
    }

    // Returns border size relative to cell size
    inline float getBorderSize(const RecastSettings& settings)
    {
        return static_cast<float>(settings.mBorderSize) * settings.mCellSize;
    }

    inline float getRealTileSize(const RecastSettings& settings)
    {
        return settings.mTileSize * settings.mCellSize / settings.mRecastScaleFactor;
    }

    inline float getMaxNavmeshAreaRadius(const Settings& settings)
    {
        return std::floor(std::sqrt(settings.mMaxTilesNumber / osg::PIf)) - 1;
    }

    // Returns tile bounds in real coordinates
    inline TileBounds makeRealTileBoundsWithBorder(const RecastSettings& settings, const TilePosition& tilePosition)
    {
        TileBounds result = makeTileBounds(settings, tilePosition);
        const float border = getBorderSize(settings);
        result.mMin -= osg::Vec2f(border, border);
        result.mMax += osg::Vec2f(border, border);
        result.mMin /= settings.mRecastScaleFactor;
        result.mMax /= settings.mRecastScaleFactor;
        return result;
    }
}

#endif
