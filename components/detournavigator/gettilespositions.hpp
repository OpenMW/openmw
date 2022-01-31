#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_GETTILESPOSITIONS_H

#include "tileposition.hpp"

class btVector3;
class btTransform;
class btCollisionShape;

namespace osg
{
    class Vec2f;
}

namespace DetourNavigator
{
    struct RecastSettings;

    struct TilesPositionsRange
    {
        TilePosition mBegin;
        TilePosition mEnd;
    };

    TilesPositionsRange makeTilesPositionsRange(const osg::Vec2f& aabbMin,
        const osg::Vec2f& aabbMax, const RecastSettings& settings);

    TilesPositionsRange makeTilesPositionsRange(const btCollisionShape& shape,
        const btTransform& transform, const RecastSettings& settings);

    TilesPositionsRange makeTilesPositionsRange(const int cellSize, const btVector3& shift,
        const RecastSettings& settings);

    template <class Callback>
    inline void getTilesPositions(const TilesPositionsRange& range, Callback&& callback)
    {
        for (int tileX = range.mBegin.x(); tileX < range.mEnd.x(); ++tileX)
            for (int tileY = range.mBegin.y(); tileY < range.mEnd.y(); ++tileY)
                callback(TilePosition {tileX, tileY});
    }
}

#endif
