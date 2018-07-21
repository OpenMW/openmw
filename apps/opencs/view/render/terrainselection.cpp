#include "terrainselection.hpp"

#include <algorithm>

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>

#include "../../model/world/cellcoordinates.hpp"

#include "worldspacewidget.hpp"

namespace
{
    const int cellSize {ESM::Land::REAL_SIZE};
    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};
}

CSVRender::TerrainSelection::TerrainSelection(osg::Group* parentNode):
mParentNode(parentNode)
{
    activate();

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // Color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array{};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    geometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    // Primitive
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays{osg::PrimitiveSet::Mode::LINES};

    geometry->addPrimitiveSet(drawArrays);
    geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    mGeode = new osg::Geode();
    mGeode->addDrawable(geometry);
    mParentNode->addChild(mGeode);
}

CSVRender::TerrainSelection::~TerrainSelection()
{
    deactivate();
}

void CSVRender::TerrainSelection::selectTerrainTexture(const WorldspaceHitResult& hit)
{
    int cellX = static_cast<int> (std::floor (hit.worldPos.x() / ESM::Land::REAL_SIZE));
    int cellY = static_cast<int> (std::floor (hit.worldPos.y() / ESM::Land::REAL_SIZE));

    mSelection.clear();

    const std::pair<int, int> localPos {toTextureCoords(hit.worldPos)};

    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
        mSelection.push_back(localPos);

    update();
}

void CSVRender::TerrainSelection::onlyAddSelect(const WorldspaceHitResult& hit)
{
    const std::pair<int, int> localPos {toTextureCoords(hit.worldPos)};

    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
            mSelection.push_back(localPos);

    update();
}

void CSVRender::TerrainSelection::toggleSelect(const WorldspaceHitResult& hit)
{
    const std::pair<int, int> localPos {toTextureCoords(hit.worldPos)};
    const auto iter = std::find(mSelection.begin(), mSelection.end(), localPos);

    if (iter != mSelection.end()) {
        mSelection.erase(iter);
    }
    else {
        mSelection.push_back(localPos);
    }
    update();
}

void CSVRender::TerrainSelection::activate()
{
    //mParentNode->addChild(mBaseNode);
}

void CSVRender::TerrainSelection::deactivate()
{
    //mParentNode->removeChild(mBaseNode);
}

std::pair<int, int> CSVRender::TerrainSelection::toTextureCoords(osg::Vec3d worldPos) const
{
    const double xd {worldPos.x() * landTextureSize / cellSize+(landTextureSize / cellSize)/4};
    const double yd {worldPos.y() * landTextureSize / cellSize+(landTextureSize / cellSize)/4};

    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    return std::make_pair(x, y);
}

std::pair<int, int> CSVRender::TerrainSelection::toVertexCoords(osg::Vec3d worldPos) const
{
    // Old code commented here
    /*
    const auto x = static_cast<int>(std::floor(toCellCoords(worldPos).first + 0.5));
    const auto y = static_cast<int>(std::floor(toCellCoords(worldPos).second + 0.5));

    return std::make_pair(x, y);
    */
    return std::make_pair(0, 0); // Not implemented yet
}

bool CSVRender::TerrainSelection::isInCell(const WorldspaceHitResult& hit) const // not used!
{
    if (!hit.hit)
        return false;

    const int cellX {mCoords.getX() * cellSize};
    const int cellY {mCoords.getY() * cellSize};

    return hit.worldPos.x() >= cellX && hit.worldPos.x() < cellX + cellSize
        && hit.worldPos.y() >= cellY && hit.worldPos.y() < cellY + cellSize;
}

double CSVRender::TerrainSelection::toWorldCoords(int pos)
{
    return cellSize * static_cast<double>(pos) / landTextureSize;
}

size_t CSVRender::TerrainSelection::landIndex(int x, int y)
{
    return y * landSize + x;
}

void CSVRender::TerrainSelection::update()
{
    osg::ref_ptr<osg::Geometry> newGeometry = new osg::Geometry();

    // Color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array{};
    colors->push_back(osg::Vec4f(0.f, 0.5f, 0.f, 1.f));

    newGeometry->setColorArray(colors, osg::Array::Binding::BIND_OVERALL);

    newGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::Values::OFF);

    const osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array);

    if (!mSelection.empty())
    {

        for (std::pair<int, int> localPos : mSelection) {
            int x {localPos.first};
            int y {localPos.second};

            // check adjacent terrain squares to see if there should be a border between them
            const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
            const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
            const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
            const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));
            const int textureSizeToLandSizeModifier = 4;

            // Nudge selection by 1/4th of a texture size, similar how blendmaps are nudged
            const int nudgeOffset = (ESM::Land::REAL_SIZE / ESM::Land::LAND_TEXTURE_SIZE)/4;
            const int landHeightsNudge = (ESM::Land::REAL_SIZE / ESM::Land::LAND_SIZE)/64; // Does this work with all land size configurations?

            int x1 = x * textureSizeToLandSizeModifier+landHeightsNudge;
            int x2 = x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier + landHeightsNudge;
            int y1 = y * textureSizeToLandSizeModifier - landHeightsNudge;
            int y2 = y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier - landHeightsNudge;

            int landHeightX1Y1 = calculateLandHeight(x1, y1),
                landHeightX1Y2 = calculateLandHeight(x1, y2),
                landHeightX2Y1 = calculateLandHeight(x2, y1),
                landHeightX2Y2 = calculateLandHeight(x2, y2);

            if (north == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX1Y2+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX2Y2+5));
            }

            if (south == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX1Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX2Y1+5));
            }

            if (east == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX2Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX2Y2+5));
            }

            if (west == mSelection.end()) {
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y) - nudgeOffset, landHeightX1Y1+5));
                vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, landHeightX1Y2+5));
            }
        }
    }

    newGeometry->setVertexArray(vertices);

    const auto drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

    drawArrays->setCount(vertices->size());

    newGeometry->addPrimitiveSet(drawArrays);

    mGeode->setDrawable(0, newGeometry);
}

int CSVRender::TerrainSelection::calculateLandHeight(int x, int y)
{
    return 1000; // To-do: calculate actual land height

    // Old land fetch code below
    /*const ESM::Land::LandData* landData (getLandData());

    if (!landData)
        return 250;

    enum cellTargetType {thisCell, rightCell, upCell, upRightCell};
    cellTargetType cellTarget = thisCell;

    if (x > ESM::Land::LAND_SIZE - 1 && y < 0)
    {
        cellTarget = upRightCell;
        x = x - ESM::Land::LAND_SIZE + 1;
        y = y + ESM::Land::LAND_SIZE - 1;
    }
    if (x > ESM::Land::LAND_SIZE - 1)
    {
        cellTarget = rightCell;
        x = x - ESM::Land::LAND_SIZE + 1;
    }
    if (y < 0)
    {
        cellTarget = upCell;
        y = y + ESM::Land::LAND_SIZE - 1;
    }

    switch(cellTarget)
    {
        case thisCell : return landData->mHeights[landIndex(x,y)];
        //case rightCell : return mEsmLandRightCell->mHeights[landIndex(x,y)]; // TO-DO: Get height from neighbouring cell
        //case upCell: return mEsmLandUpCell->mHeights[landIndex(x,y)]; // TO-DO: Get height from neighbouring cell
        //case upRightCell: return mEsmLandUpRightCell->mHeights[landIndex(x,y)];; // TO-DO: Get height from neighbouring cell
    }*/
    return 0;
}

// To-do: Implement a get function
/*CSVRender::TerrainSelection CSVRender::Cell::getTerrainSelection(TerrainSelectionType type) const
{
    switch (type) {
        case TerrainSelectionType::Texture:
            return mTerrainTextureSelection.get();
        // other types of terrain can go here
        default:
            return nullptr;
    }
}*/
