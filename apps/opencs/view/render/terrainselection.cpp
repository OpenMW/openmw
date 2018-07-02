#include "terrainselection.hpp"

#include <osg/Group>

#include "../../model/world/cellcoordinates.hpp"

#include "worldspacewidget.hpp"

namespace
{
    const int cellSize {ESM::Land::REAL_SIZE};
    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};
}

CSVRender::TerrainSelection::TerrainSelection(const CSMWorld::CellCoordinates& coords, const ESM::Land& esmLand, osg::Group* parentNode)
    :mCoords (coords), mEsmLand (esmLand), mParentNode (parentNode)
{
    mBaseNode = new osg::PositionAttitudeTransform{};
    mBaseNode->setPosition(osg::Vec3d(coords.getX() * cellSize, coords.getY() * cellSize, 10.0));
    activate();
}

CSVRender::TerrainSelection::~TerrainSelection()
{
    deactivate();
}

void CSVRender::TerrainSelection::select(const WorldspaceHitResult& hit)
{
    deselect();

    if (isInCell(hit)) {
        addToSelection(hit.worldPos);
    }
    update();
}

void CSVRender::TerrainSelection::onlyAddSelect(const WorldspaceHitResult& hit)
{
    if (isInCell(hit)) {
        addToSelection(hit.worldPos);
    }
    update();
}

void CSVRender::TerrainSelection::toggleSelect(const WorldspaceHitResult& hit)
{
    if (isInCell(hit)) {
        toggleSelection(hit.worldPos);
        update();
    }
}

void CSVRender::TerrainSelection::activate()
{
    mParentNode->addChild(mBaseNode);
}

void CSVRender::TerrainSelection::deactivate()
{
    mParentNode->removeChild(mBaseNode);
}

CSVRender::TerrainSelection::TerrainPos CSVRender::TerrainSelection::toTextureCoords(osg::Vec3d worldPos) const
{
    const auto x = static_cast<int>(std::floor(toCellCoords(worldPos).first));
    const auto y = static_cast<int>(std::floor(toCellCoords(worldPos).second));

    return std::make_pair(x, y);
}

CSVRender::TerrainSelection::TerrainPos CSVRender::TerrainSelection::toVertexCoords(osg::Vec3d worldPos) const
{
    const auto x = static_cast<int>(std::floor(toCellCoords(worldPos).first + 0.5));
    const auto y = static_cast<int>(std::floor(toCellCoords(worldPos).second + 0.5));

    return std::make_pair(x, y);
}

const ESM::Land::LandData* CSVRender::TerrainSelection::getLandData() const
{
    return mEsmLand.getLandData(ESM::Land::DATA_VHGT);
}

const osg::ref_ptr<osg::PositionAttitudeTransform>& CSVRender::TerrainSelection::getBaseNode() const
{
    return mBaseNode;
}

bool CSVRender::TerrainSelection::isInCell(const WorldspaceHitResult& hit) const
{
    if (!hit.hit)
        return false;

    const int cellX {mCoords.getX() * cellSize};
    const int cellY {mCoords.getY() * cellSize};

    return hit.worldPos.x() >= cellX && hit.worldPos.x() < cellX + cellSize
        && hit.worldPos.y() >= cellY && hit.worldPos.y() < cellY + cellSize;
}

std::pair<double, double> CSVRender::TerrainSelection::toCellCoords(osg::Vec3d worldPos) const
{
    const int cellX {mCoords.getX() * cellSize};
    const int cellY {mCoords.getY() * cellSize};

    const double x {(worldPos.x() - cellX) * landTextureSize / cellSize};
    const double y {(worldPos.y() - cellY) * landTextureSize / cellSize};

    return std::make_pair(x, y);
}

double CSVRender::TerrainSelection::toWorldCoords(int pos)
{
    return (cellSize + 128) * static_cast<double>(pos) / landTextureSize;
}

size_t CSVRender::TerrainSelection::landIndex(int x, int y)
{
    return y * landSize + x;
}
