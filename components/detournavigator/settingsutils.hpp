#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGSUTILS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SETTINGSUTILS_H

#include "settings.hpp"
#include "tilebounds.hpp"
#include "tileposition.hpp"
#include "tilebounds.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2f>
#include <osg/Vec2i>
#include <osg/Vec3f>

#include <utility>

namespace DetourNavigator
{
    inline float getHeight(const Settings& settings,const osg::Vec3f& agentHalfExtents)
    {
        return 2.0f * agentHalfExtents.z() * settings.mRecastScaleFactor;
    }

    inline float getMaxClimb(const Settings& settings)
    {
        return settings.mMaxClimb * settings.mRecastScaleFactor;
    }

    inline float getRadius(const Settings& settings, const osg::Vec3f& agentHalfExtents)
    {
        return agentHalfExtents.x() * settings.mRecastScaleFactor;
    }

    inline float toNavMeshCoordinates(const Settings& settings, float value)
    {
        return value * settings.mRecastScaleFactor;
    }

    inline osg::Vec3f toNavMeshCoordinates(const Settings& settings, osg::Vec3f position)
    {
        std::swap(position.y(), position.z());
        return position * settings.mRecastScaleFactor;
    }

    inline osg::Vec3f fromNavMeshCoordinates(const Settings& settings, osg::Vec3f position)
    {
        const auto factor = 1.0f / settings.mRecastScaleFactor;
        position *= factor;
        std::swap(position.y(), position.z());
        return position;
    }

    inline float getTileSize(const Settings& settings)
    {
        return settings.mTileSize * settings.mCellSize;
    }

    inline TilePosition getTilePosition(const Settings& settings, const osg::Vec3f& position)
    {
        return TilePosition(
            static_cast<int>(std::floor(position.x() / getTileSize(settings))),
            static_cast<int>(std::floor(position.z() / getTileSize(settings)))
        );
    }

    inline TileBounds makeTileBounds(const Settings& settings, const TilePosition& tilePosition)
    {
        return TileBounds {
            osg::Vec2f(tilePosition.x(), tilePosition.y()) * getTileSize(settings),
            osg::Vec2f(tilePosition.x() + 1, tilePosition.y() + 1) * getTileSize(settings),
        };
    }

    inline float getBorderSize(const Settings& settings)
    {
        return settings.mBorderSize * settings.mCellSize;
    }

    inline float getSwimLevel(const Settings& settings, const float agentHalfExtentsZ)
    {
        return - settings.mSwimHeightScale * agentHalfExtentsZ;
    }

    inline btTransform getSwimLevelTransform(const Settings& settings, const btTransform& transform,
        const float agentHalfExtentsZ)
    {
        return btTransform(
            transform.getBasis(),
            transform.getOrigin() + btVector3(0, 0, getSwimLevel(settings, agentHalfExtentsZ) - agentHalfExtentsZ)
        );
    }
}

#endif
