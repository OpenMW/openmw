#include "terrainselection.hpp"

#include <algorithm>

#include <osg/Group>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>

#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/columnimp.hpp"
#include "../../model/world/idtable.hpp"

#include "cell.hpp"
#include "worldspacewidget.hpp"

CSVRender::TerrainSelection::TerrainSelection(osg::Group* parentNode, WorldspaceWidget *worldspaceWidget, TerrainSelectionType type):
mParentNode(parentNode), mWorldspaceWidget (worldspaceWidget), mDraggedOperationFlag(false), mSelectionType(type)
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

void CSVRender::TerrainSelection::onlySelect(const std::vector<std::pair<int, int>> &localPositions)
{
    mSelection = localPositions;
    update();
}

void CSVRender::TerrainSelection::addSelect(const std::pair<int, int> &localPos)
{
    if (std::find(mSelection.begin(), mSelection.end(), localPos) == mSelection.end())
    {
        mSelection.emplace_back(localPos);
        update();
    }
}

void CSVRender::TerrainSelection::toggleSelect(const std::vector<std::pair<int, int>> &localPositions, bool toggleInProgress)
{
    if (toggleInProgress)
    {
        for(auto const& localPos: localPositions)
        {
            auto iterTemp = std::find(mTemporarySelection.begin(), mTemporarySelection.end(), localPos);
            mDraggedOperationFlag = true;

            if (iterTemp == mTemporarySelection.end())
            {
                auto iter = std::find(mSelection.begin(), mSelection.end(), localPos);
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
    if (!mParentNode->containsNode(mSelectionNode)) mParentNode->addChild(mSelectionNode);
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

    switch (mSelectionType)
    {
        case TerrainSelectionType::Texture : drawTextureSelection(vertices);
        break;
        case TerrainSelectionType::Shape : drawShapeSelection(vertices);
        break;
    }

    mGeometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);
    drawArrays->setCount(vertices->size());
    if (vertices->size() != 0) mGeometry->addPrimitiveSet(drawArrays);
    mSelectionNode->addChild(mGeometry);
}

void CSVRender::TerrainSelection::drawShapeSelection(const osg::ref_ptr<osg::Vec3Array> vertices)
{
    if (!mSelection.empty())
    {
        for (std::pair<int, int> &localPos : mSelection)
        {
            int x (localPos.first);
            int y (localPos.second);

            float xWorldCoord(CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(x));
            float yWorldCoord(CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(y));

            osg::Vec3f pointXY(xWorldCoord, yWorldCoord, calculateLandHeight(x, y) + 2);

            vertices->push_back(pointXY);
            vertices->push_back(osg::Vec3f(xWorldCoord, CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(y - 1), calculateLandHeight(x, y - 1) + 2));
            vertices->push_back(pointXY);
            vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(x - 1), yWorldCoord, calculateLandHeight(x - 1, y) + 2));

            const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
            if (north == mSelection.end())
            {
                vertices->push_back(pointXY);
                vertices->push_back(osg::Vec3f(xWorldCoord, CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(y + 1), calculateLandHeight(x, y + 1) + 2));
            }

            const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
            if (east == mSelection.end())
            {
                vertices->push_back(pointXY);
                vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::vertexGlobalToWorldCoords(x + 1), yWorldCoord, calculateLandHeight(x + 1, y) + 2));
            }
        }
    }
}

void CSVRender::TerrainSelection::drawTextureSelection(const osg::ref_ptr<osg::Vec3Array> vertices)
{
    if (!mSelection.empty())
    {
        const int landHeightsNudge = (ESM::Land::REAL_SIZE / ESM::Land::LAND_SIZE) / (ESM::Land::LAND_SIZE - 1); // Does this work with all land size configurations?

        const int textureSizeToLandSizeModifier = (ESM::Land::LAND_SIZE - 1) / ESM::Land::LAND_TEXTURE_SIZE;

        for (std::pair<int, int> &localPos : mSelection)
        {
            int x (localPos.first);
            int y (localPos.second);

            // convert texture selection to global vertex coordinates at selection box corners
            int x1 = x * textureSizeToLandSizeModifier + landHeightsNudge;
            int x2 = x * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier + landHeightsNudge;
            int y1 = y * textureSizeToLandSizeModifier - landHeightsNudge;
            int y2 = y * textureSizeToLandSizeModifier + textureSizeToLandSizeModifier - landHeightsNudge;

            // Draw edges (check all sides, draw lines between vertices, +1 height to keep lines above ground)
            // Check adjancent selections, draw lines only to edges of the selection
            const auto north = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y + 1));
            if (north == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    float drawPreviousX = CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x) + (i - 1) * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    float drawCurrentX = CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x) + i * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX, CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y + 1), calculateLandHeight(x1+(i-1), y2)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX, CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y + 1), calculateLandHeight(x1+i, y2)+2));
                }
            }

            const auto south = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x, y - 1));
            if (south == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    float drawPreviousX = CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x) + (i - 1) *(ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    float drawCurrentX = CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x) + i * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    vertices->push_back(osg::Vec3f(drawPreviousX, CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y), calculateLandHeight(x1+(i-1), y1)+2));
                    vertices->push_back(osg::Vec3f(drawCurrentX, CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y), calculateLandHeight(x1+i, y1)+2));
                }
            }

            const auto east = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x + 1, y));
            if (east == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    float drawPreviousY = CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y) + (i - 1) * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    float drawCurrentY = CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y) + i * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x + 1), drawPreviousY, calculateLandHeight(x2, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x + 1), drawCurrentY, calculateLandHeight(x2, y1+i)+2));
                }
            }

            const auto west = std::find(mSelection.begin(), mSelection.end(), std::make_pair(x - 1, y));
            if (west == mSelection.end())
            {
                for(int i = 1; i < (textureSizeToLandSizeModifier + 1); i++)
                {
                    float drawPreviousY = CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y) + (i - 1) * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    float drawCurrentY = CSMWorld::CellCoordinates::textureGlobalYToWorldCoords(y) + i * (ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x), drawPreviousY, calculateLandHeight(x1, y1+(i-1))+2));
                    vertices->push_back(osg::Vec3f(CSMWorld::CellCoordinates::textureGlobalXToWorldCoords(x), drawCurrentY, calculateLandHeight(x1, y1+i)+2));
                }
            }
        }
    }
}

bool CSVRender::TerrainSelection::noCell(const std::string& cellId)
{
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    const CSMWorld::IdCollection<CSMWorld::Cell>& cellCollection = document.getData().getCells();
    return cellCollection.searchId (cellId) == -1;
}

bool CSVRender::TerrainSelection::noLand(const std::string& cellId)
{
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    const CSMWorld::IdCollection<CSMWorld::Land>& landCollection = document.getData().getLand();
    return landCollection.searchId (cellId) == -1;
}

bool CSVRender::TerrainSelection::noLandLoaded(const std::string& cellId)
{
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    const CSMWorld::IdCollection<CSMWorld::Land>& landCollection = document.getData().getLand();
    return !landCollection.getRecord(cellId).get().isDataLoaded(ESM::Land::DATA_VNML);
}

bool CSVRender::TerrainSelection::isLandLoaded(const std::string& cellId)
{
    if (!noCell(cellId) && !noLand(cellId) && !noLandLoaded(cellId)) return true;
    return false;
}

int CSVRender::TerrainSelection::calculateLandHeight(int x, int y) // global vertex coordinates
{
    int cellX = std::floor(static_cast<float>(x) / (ESM::Land::LAND_SIZE - 1));
    int cellY = std::floor(static_cast<float>(y) / (ESM::Land::LAND_SIZE - 1));
    int localX = x - cellX * (ESM::Land::LAND_SIZE - 1);
    int localY = y - cellY * (ESM::Land::LAND_SIZE - 1);

    CSMWorld::CellCoordinates coords (cellX, cellY);

    float landHeight = 0.f;
    if (CSVRender::Cell* cell = dynamic_cast<CSVRender::Cell*>(mWorldspaceWidget->getCell(coords)))
    {
        landHeight = cell->getSumOfAlteredAndTrueHeight(cellX, cellY, localX, localY);
    }
    else if (isLandLoaded(CSMWorld::CellCoordinates::generateId(cellX, cellY)))
    {
        CSMDoc::Document& document = mWorldspaceWidget->getDocument();
        CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> ( *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
        std::string cellId = CSMWorld::CellCoordinates::generateId(cellX, cellY);
        int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
        const CSMWorld::LandHeightsColumn::DataType mPointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        return mPointer[localY*ESM::Land::LAND_SIZE + localX];
    }

    return landHeight;
}
