#include "terrainselection.hpp"

#include <algorithm>

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>

#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/columnimp.hpp"
#include "../../model/world/idtable.hpp"

#include "worldspacewidget.hpp"

namespace
{
    const int cellSize {ESM::Land::REAL_SIZE};
    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};
}

CSVRender::TerrainSelection::TerrainSelection(osg::Group* parentNode, WorldspaceWidget *worldspaceWidget):
mParentNode(parentNode), mWorldspaceWidget (worldspaceWidget)
{
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

    activate();
}

CSVRender::TerrainSelection::~TerrainSelection()
{
    deactivate();
}

std::vector<std::pair<int, int>> CSVRender::TerrainSelection::getTerrainSelection() const
{
    return mSelection;
}

void CSVRender::TerrainSelection::selectTerrainTexture(const WorldspaceHitResult& hit)
{
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
    mParentNode->addChild(mGeode);
}

void CSVRender::TerrainSelection::deactivate()
{
    mParentNode->removeChild(mGeode);
}

std::pair<int, int> CSVRender::TerrainSelection::toTextureCoords(osg::Vec3d worldPos) const
{
    const double xd {worldPos.x() * landTextureSize / cellSize - 0.25f};
    const double yd {worldPos.y() * landTextureSize / cellSize + 0.25f};

    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    return std::make_pair(x, y);
}

std::pair<int, int> CSVRender::TerrainSelection::toVertexCoords(osg::Vec3d worldPos) const
{
    const double xd (worldPos.x() * landSize / cellSize);
    const double yd (worldPos.y() * landSize / cellSize);

    const auto x = static_cast<int>(std::floor(xd));
    const auto y = static_cast<int>(std::floor(yd));

    return std::make_pair(x, y);
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
    colors->push_back(osg::Vec4f(1.f, 1.f, 1.f, 1.f));

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
            const int textureSizeToLandSizeModifier = (landSize - 1) / landTextureSize;

            // Nudge selection by 1/4th of a texture size, similar how blendmaps are nudged
            const float NudgePercentage = 0.25f;
            const int nudgeOffset = (cellSize / landTextureSize) * NudgePercentage;
            const int landHeightsNudge = (cellSize / landSize)/ (landSize - 1); // Does this work with all land size configurations?

            // calculate global vertex coordinates at selection box corners
            int x1 = x * textureSizeToLandSizeModifier + landHeightsNudge;
            int x2 = x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier + landHeightsNudge;
            int y1 = y * textureSizeToLandSizeModifier - landHeightsNudge;
            int y2 = y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier - landHeightsNudge;

            // Draw edges (check all sides, draw lines between vertices, +1 height to keep lines above ground)
            if (north == mSelection.end()) {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousX = toWorldCoords(x)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentX = toWorldCoords(x)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, calculateLandHeight(x1+(i-1), y2)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX + nudgeOffset, toWorldCoords(y + 1) - nudgeOffset, calculateLandHeight(x1+i, y2)+2));
                }
            }

            if (south == mSelection.end()) {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousX = toWorldCoords(x)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentX = toWorldCoords(x)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX + nudgeOffset, toWorldCoords(y) - nudgeOffset, calculateLandHeight(x1+(i-1), y1)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX + nudgeOffset, toWorldCoords(y) - nudgeOffset, calculateLandHeight(x1+i, y1)+2));
                }
            }

            if (east == mSelection.end()) {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousY = toWorldCoords(y)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentY = toWorldCoords(y)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, drawPreviousY - nudgeOffset, calculateLandHeight(x2, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(toWorldCoords(x + 1) + nudgeOffset, drawCurrentY - nudgeOffset, calculateLandHeight(x2, y1+i)+2));
                }
            }

            if (west == mSelection.end()) {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousY = toWorldCoords(y)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentY = toWorldCoords(y)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, drawPreviousY - nudgeOffset, calculateLandHeight(x1, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(toWorldCoords(x) + nudgeOffset, drawCurrentY - nudgeOffset, calculateLandHeight(x1, y1+i)+2));
                }
            }
        }
    }

    newGeometry->setVertexArray(vertices);

    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

    drawArrays->setCount(vertices->size());

    newGeometry->addPrimitiveSet(drawArrays);

    mGeode->setDrawable(0, newGeometry);
}

int CSVRender::TerrainSelection::calculateLandHeight(int x, int y) // global vertex coordinates
{
    int cellX(0);
    int cellY(0);
    int localX(0);
    int localY(0);

    if (x >= 0)
    {
        cellX = std::floor(x / (landSize - 1));
        localX = x - cellX * (landSize - 1);
    }
    if (y >= 0)
    {
        cellY = std::floor(y / (landSize - 1));
        localY = y - cellY * (landSize - 1);
    }
    if (x < 0)
    {
        cellX = std::trunc(x / (landSize - 1)) - 1;
        localX = x - cellX * (landSize - 1);
    }
    if (y < 0)
    {
        cellY = std::trunc(y / (landSize - 1)) - 1;
        localY = y - cellY * (landSize - 1);
    }

    std::string cellId = "#" + std::to_string(cellX) + " " + std::to_string(cellY);

    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType mPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

    return mPointer[localY*landSize + localX];
}
