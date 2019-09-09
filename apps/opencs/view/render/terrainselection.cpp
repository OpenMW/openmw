#include "terrainselection.hpp"

#include <algorithm>

#include <osg/Group>
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
mParentNode(parentNode), mWorldspaceWidget (worldspaceWidget), mDraggedOperationFlag(false)
{
    mGeometry = new osg::Geometry();

    mSelectionNode = new osg::Group();
    mSelectionNode->addChild(mGeometry);

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

void CSVRender::TerrainSelection::onlySelect(const std::vector<std::pair<int, int>> localPositions)
{
    mSelection.clear();
    for(auto const& value: localPositions)
    {
        mSelection.emplace_back(value);
    }
    update();
}

void CSVRender::TerrainSelection::addSelect(const std::pair<int, int> localPos)
{
    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
            mSelection.emplace_back(localPos);

    update();
}

void CSVRender::TerrainSelection::toggleSelect(const std::vector<std::pair<int, int>> localPositions, bool toggleInProgress)
{
    if (toggleInProgress == true)
    {
        for(auto const& localPos: localPositions)
        {
            auto iter = std::find(mSelection.begin(), mSelection.end(), localPos);
            auto itertemp = std::find(mTemporarySelection.begin(), mTemporarySelection.end(), localPos);
            mDraggedOperationFlag = true;

            if (itertemp == mTemporarySelection.end())
            {
                if (iter != mSelection.end())
                {
                    mSelection.erase(iter);
                }
                else
                {
                    mSelection.emplace_back(localPos);
                }
            }

            mTemporarySelection.push_back(localPos);
        }
    }
    else if (mDraggedOperationFlag == false)
    {
        for(auto const& localPos: localPositions)
        {
            const auto iter = std::find(mSelection.begin(), mSelection.end(), localPos);
            if (iter != mSelection.end())
            {
                mSelection.erase(iter);
            }
            else
            {
                mSelection.emplace_back(localPos);
            }
        }
    }
    else
    {
        mDraggedOperationFlag = false;
        mTemporarySelection.clear();
    }
    update();
}

void CSVRender::TerrainSelection::activate()
{
    mParentNode->addChild(mSelectionNode);
}

void CSVRender::TerrainSelection::deactivate()
{
    mParentNode->removeChild(mSelectionNode);
}

void CSVRender::TerrainSelection::update()
{
    mSelectionNode->removeChild(mGeometry);
    mGeometry = new osg::Geometry();

    const osg::ref_ptr<osg::Vec3Array> vertices (new osg::Vec3Array);

    if (!mSelection.empty())
    {

        for (std::pair<int, int> localPos : mSelection)
        {
            int x {localPos.first};
            int y {localPos.second};

            // check adjacent terrain squares to see if there should be a border between them
            const int textureSizeToLandSizeModifier = (landSize - 1) / landTextureSize;

            // Nudge selection by 1/4th of a texture size, similar how blendmaps are nudged
            const float nudgePercentage = 0.25f;
            const int nudgeOffset = (cellSize / landTextureSize) * nudgePercentage;
            const int landHeightsNudge = (cellSize / landSize) / (landSize - 1); // Does this work with all land size configurations?

            // convert texture selection to global vertex coordinates at selection box corners
            int x1 = x * textureSizeToLandSizeModifier + landHeightsNudge;
            int x2 = x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier + landHeightsNudge;
            int y1 = y * textureSizeToLandSizeModifier - landHeightsNudge;
            int y2 = y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier - landHeightsNudge;

            // Draw edges (check all sides, draw lines between vertices, +1 height to keep lines above ground)
            const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
            if (north == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousX = CSMWorld::CellCoordinates::texSelectionToWorldCoords(x)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentX = CSMWorld::CellCoordinates::texSelectionToWorldCoords(x)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX + nudgeOffset, CSMWorld::CellCoordinates::texSelectionToWorldCoords(y + 1) - nudgeOffset, calculateLandHeight(x1+(i-1), y2)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX + nudgeOffset, CSMWorld::CellCoordinates::texSelectionToWorldCoords(y + 1) - nudgeOffset, calculateLandHeight(x1+i, y2)+2));
                }
            }

            const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
            if (south == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousX = CSMWorld::CellCoordinates::texSelectionToWorldCoords(x)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentX = CSMWorld::CellCoordinates::texSelectionToWorldCoords(x)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX + nudgeOffset, CSMWorld::CellCoordinates::texSelectionToWorldCoords(y) - nudgeOffset, calculateLandHeight(x1+(i-1), y1)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX + nudgeOffset, CSMWorld::CellCoordinates::texSelectionToWorldCoords(y) - nudgeOffset, calculateLandHeight(x1+i, y1)+2));
                }
            }

            const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
            if (east == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousY = CSMWorld::CellCoordinates::texSelectionToWorldCoords(y)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentY = CSMWorld::CellCoordinates::texSelectionToWorldCoords(y)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::texSelectionToWorldCoords(x + 1) + nudgeOffset, drawPreviousY - nudgeOffset, calculateLandHeight(x2, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::texSelectionToWorldCoords(x + 1) + nudgeOffset, drawCurrentY - nudgeOffset, calculateLandHeight(x2, y1+i)+2));
                }
            }

            const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));
            if (west == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    double drawPreviousY = CSMWorld::CellCoordinates::texSelectionToWorldCoords(y)+(i-1)*(cellSize / (landSize - 1));
                    double drawCurrentY = CSMWorld::CellCoordinates::texSelectionToWorldCoords(y)+i*(cellSize / (landSize - 1));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::texSelectionToWorldCoords(x) + nudgeOffset, drawPreviousY - nudgeOffset, calculateLandHeight(x1, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::texSelectionToWorldCoords(x) + nudgeOffset, drawCurrentY - nudgeOffset, calculateLandHeight(x1, y1+i)+2));
                }
            }
        }
    }

    mGeometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);
    drawArrays->setCount(vertices->size());
    mGeometry->addPrimitiveSet(drawArrays);
    mSelectionNode->addChild(mGeometry);
}

int CSVRender::TerrainSelection::calculateLandHeight(int x, int y) // global vertex coordinates
{
    int cellX = std::floor((1.0f*x / (landSize - 1)));
    int cellY = std::floor((1.0f*y / (landSize - 1)));
    int localX = x - cellX * (landSize - 1);
    int localY = y - cellY * (landSize - 1);

    std::string cellId = CSMWorld::CellCoordinates::generateId(cellX, cellY);

    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType mPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

    return mPointer[localY*landSize + localX];
}
