#include "positioncellgrid.hpp"

#include <cmath>

#include <components/esm/util.hpp>

namespace MWWorld
{
    std::vector<PositionCellGrid> terrainPreloadPositions(const osg::Vec3f& primary, const osg::Vec3f& playerPos,
        const osg::Vec3f& cameraPos, bool cameraDetached, const osg::Vec4i& grid, ESM::RefId worldspace)
    {
        std::vector<PositionCellGrid> positions{ PositionCellGrid{ primary, grid } };
        if (!cameraDetached)
            return positions;
        // Camera distance bounds terrain LOD preloads.
        const float distance = (cameraPos - playerPos).length();
        if (distance >= terrainPreloadMergeDistance && distance < static_cast<float>(ESM::getCellSize(worldspace)))
        {
            // Snapping keeps orbit preloads from restarting.
            constexpr float quantum = terrainPreloadMergeDistance;
            const osg::Vec3f snapped(std::floor(cameraPos.x() / quantum) * quantum,
                std::floor(cameraPos.y() / quantum) * quantum, std::floor(cameraPos.z() / quantum) * quantum);
            positions.push_back(PositionCellGrid{ snapped, grid });
        }
        return positions;
    }
}
