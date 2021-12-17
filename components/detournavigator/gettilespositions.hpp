#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H

#include "tileposition.hpp"

class btVector3;
class btTransform;
class btCollisionShape;

namespace osg
{
    class Vec3f;
}

namespace DetourNavigator
{
    struct RecastSettings;

    struct TilesPositionsRange
    {
        TilePosition mMin;
        TilePosition mMax;
    };

    TilesPositionsRange makeTilesPositionsRange(const osg::Vec3f& aabbMin,
        const osg::Vec3f& aabbMax, const RecastSettings& settings);

    TilesPositionsRange makeTilesPositionsRange(const btCollisionShape& shape,
        const btTransform& transform, const RecastSettings& settings);

    TilesPositionsRange makeTilesPositionsRange(const int cellSize, const btVector3& shift,
        const RecastSettings& settings);

    template <class Callback>
    void getTilesPositions(const TilesPositionsRange& range, Callback&& callback)
    {
        for (int tileX = range.mMin.x(); tileX <= range.mMax.x(); ++tileX)
            for (int tileY = range.mMin.y(); tileY <= range.mMax.y(); ++tileY)
                callback(TilePosition {tileX, tileY});
    }
}

#endif
