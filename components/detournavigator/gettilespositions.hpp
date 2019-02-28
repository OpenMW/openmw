#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H

#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include <osg/Vec3f>

namespace DetourNavigator
{
    template <class Callback>
    void getTilesPositions(const osg::Vec3f& aabbMin, const osg::Vec3f& aabbMax,
        const Settings& settings, Callback&& callback)
    {
        auto min = toNavMeshCoordinates(settings, aabbMin);
        auto max = toNavMeshCoordinates(settings, aabbMax);

        const auto border = getBorderSize(settings);
        min -= osg::Vec3f(border, border, border);
        max += osg::Vec3f(border, border, border);

        auto minTile = getTilePosition(settings, min);
        auto maxTile = getTilePosition(settings, max);

        if (minTile.x() > maxTile.x())
            std::swap(minTile.x(), maxTile.x());

        if (minTile.y() > maxTile.y())
            std::swap(minTile.y(), maxTile.y());

        for (int tileX = minTile.x(); tileX <= maxTile.x(); ++tileX)
            for (int tileY = minTile.y(); tileY <= maxTile.y(); ++tileY)
                callback(TilePosition {tileX, tileY});
    }

    template <class Callback>
    void getTilesPositions(const btCollisionShape& shape, const btTransform& transform,
        const Settings& settings, Callback&& callback)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(transform, aabbMin, aabbMax);

        getTilesPositions(Misc::Convert::makeOsgVec3f(aabbMin), Misc::Convert::makeOsgVec3f(aabbMax), settings, std::forward<Callback>(callback));
    }

    template <class Callback>
    void getTilesPositions(const int cellSize, const btTransform& transform,
        const Settings& settings, Callback&& callback)
    {
        const auto halfCellSize = cellSize / 2;
        auto aabbMin = transform(btVector3(-halfCellSize, -halfCellSize, 0));
        auto aabbMax = transform(btVector3(halfCellSize, halfCellSize, 0));

        aabbMin.setX(std::min(aabbMin.x(), aabbMax.x()));
        aabbMin.setY(std::min(aabbMin.y(), aabbMax.y()));

        aabbMax.setX(std::max(aabbMin.x(), aabbMax.x()));
        aabbMax.setY(std::max(aabbMin.y(), aabbMax.y()));

        getTilesPositions(Misc::Convert::makeOsgVec3f(aabbMin), Misc::Convert::makeOsgVec3f(aabbMax), settings, std::forward<Callback>(callback));
    }
}

#endif
