#include "gettilespositions.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "tileposition.hpp"

#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionShapes/btCollisionShape.h>

namespace DetourNavigator
{
    TilesPositionsRange makeTilesPositionsRange(const osg::Vec3f& aabbMin, const osg::Vec3f& aabbMax,
        const RecastSettings& settings)
    {
        osg::Vec3f min = toNavMeshCoordinates(settings, aabbMin);
        osg::Vec3f max = toNavMeshCoordinates(settings, aabbMax);

        const float border = getBorderSize(settings);
        min -= osg::Vec3f(border, border, border);
        max += osg::Vec3f(border, border, border);

        TilePosition minTile = getTilePosition(settings, min);
        TilePosition maxTile = getTilePosition(settings, max);

        if (minTile.x() > maxTile.x())
            std::swap(minTile.x(), maxTile.x());

        if (minTile.y() > maxTile.y())
            std::swap(minTile.y(), maxTile.y());

        return {minTile, maxTile};
    }

    TilesPositionsRange makeTilesPositionsRange(const btCollisionShape& shape, const btTransform& transform,
        const RecastSettings& settings)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(transform, aabbMin, aabbMax);

        return makeTilesPositionsRange(Misc::Convert::toOsg(aabbMin), Misc::Convert::toOsg(aabbMax), settings);
    }

    TilesPositionsRange makeTilesPositionsRange(const int cellSize, const btVector3& shift,
        const RecastSettings& settings)
    {
        using Misc::Convert::toOsg;

        const int halfCellSize = cellSize / 2;
        const btTransform transform(btMatrix3x3::getIdentity(), shift);
        btVector3 aabbMin = transform(btVector3(-halfCellSize, -halfCellSize, 0));
        btVector3 aabbMax = transform(btVector3(halfCellSize, halfCellSize, 0));

        aabbMin.setX(std::min(aabbMin.x(), aabbMax.x()));
        aabbMin.setY(std::min(aabbMin.y(), aabbMax.y()));

        aabbMax.setX(std::max(aabbMin.x(), aabbMax.x()));
        aabbMax.setY(std::max(aabbMin.y(), aabbMax.y()));

        return makeTilesPositionsRange(toOsg(aabbMin), toOsg(aabbMax), settings);
    }
}
