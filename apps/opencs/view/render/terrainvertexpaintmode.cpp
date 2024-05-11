#include "terrainvertexpaintmode.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include <QComboBox>
#include <QDropEvent>
#include <QEvent>
#include <QIcon>
#include <QWidget>

#include <osg/Camera>
#include <osg/Vec3f>

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/cellselection.hpp>
#include <apps/opencs/model/world/columnimp.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/idtable.hpp>
#include <apps/opencs/model/world/land.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/widget/brushshapes.hpp>
#include <apps/opencs/view/widget/scenetool.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadland.hpp>

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolvertexpaintbrush.hpp"

#include "../../model/prefs/state.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtree.hpp"

#include "brushdraw.hpp"
#include "commands.hpp"
#include "editmode.hpp"
#include "mask.hpp"
#include "pagedworldspacewidget.hpp"
#include "terrainselection.hpp"
#include "worldspacewidget.hpp"

class QPoint;
class QWidget;

namespace CSMWorld
{
    struct Cell;
}

namespace osg
{
    class Group;
}

CSVRender::TerrainVertexPaintMode::TerrainVertexPaintMode(
    WorldspaceWidget* worldspaceWidget, osg::Group* parentNode, QWidget* parent)
    : EditMode(worldspaceWidget, QIcon{ ":scenetoolbar/editing-terrain-vertex-paint" }, Mask_Terrain,
        "Terrain vertex paint editing", parent)
    , mParentNode(parentNode)
    , mVertexPaintEditToolColor(Qt::white)
{
}

void CSVRender::TerrainVertexPaintMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    if (!mTerrainSelection)
    {
        mTerrainSelection
            = std::make_shared<TerrainSelection>(mParentNode, &getWorldspaceWidget(), TerrainSelectionType::Shape);
    }

    if (!mVertexPaintBrushScenetool)
    {
        mVertexPaintBrushScenetool = new CSVWidget::SceneToolVertexPaintBrush(
            toolbar, "scenetoolvertexpaintbrush", getWorldspaceWidget().getDocument());
        connect(mVertexPaintBrushScenetool->mVertexPaintBrushWindow, &CSVWidget::VertexPaintBrushWindow::passBrushSize,
            this, &TerrainVertexPaintMode::setBrushSize);
        connect(mVertexPaintBrushScenetool->mVertexPaintBrushWindow, &CSVWidget::VertexPaintBrushWindow::passBrushShape,
            this, &TerrainVertexPaintMode::setBrushShape);
        connect(mVertexPaintBrushScenetool->mVertexPaintBrushWindow->mSizeSliders->mBrushSizeSlider,
            &QSlider::valueChanged, this, &TerrainVertexPaintMode::setBrushSize);
        connect(mVertexPaintBrushScenetool->mVertexPaintBrushWindow->mToolSelector,
            qOverload<int>(&QComboBox::currentIndexChanged), this, &TerrainVertexPaintMode::setVertexPaintEditTool);
        connect(mVertexPaintBrushScenetool->mVertexPaintBrushWindow->mColorButtonWidget,
            &CSVWidget::ColorButtonWidget::colorChanged, this, &TerrainVertexPaintMode::setVertexPaintColor);
    }

    if (!mBrushDraw)
        mBrushDraw = std::make_unique<BrushDraw>(mParentNode);

    EditMode::activate(toolbar);
    toolbar->addTool(mVertexPaintBrushScenetool);
}

void CSVRender::TerrainVertexPaintMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    if (mVertexPaintBrushScenetool)
    {
        toolbar->removeTool(mVertexPaintBrushScenetool);
    }

    if (mTerrainSelection)
    {
        mTerrainSelection.reset();
    }

    if (mBrushDraw)
        mBrushDraw.reset();

    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainVertexPaintMode::primaryOpenPressed(const WorldspaceHitResult& hit) // Apply changes here
{
}

void CSVRender::TerrainVertexPaintMode::primaryEditPressed(const WorldspaceHitResult& hit)
{
    if (hit.hit && hit.tag == nullptr)
    {
        if (mDragMode == InteractionType_PrimaryEdit)
        {
            CSMDoc::Document& document = getWorldspaceWidget().getDocument();
            // QUndoStack& undoStack = document.getUndoStack();
            // undoStack.beginMacro("Set land normals");
            editVertexColourGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
        }
    }
    endVertexPaintEditing();
}

void CSVRender::TerrainVertexPaintMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    if (hit.hit && hit.tag == nullptr)
    {
        selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0);
        mTerrainSelection->clearTemporarySelection();
    }
}

void CSVRender::TerrainVertexPaintMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
    if (hit.hit && hit.tag == nullptr)
    {
        selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1);
        mTerrainSelection->clearTemporarySelection();
    }
}

bool CSVRender::TerrainVertexPaintMode::primaryEditStartDrag(const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());

    mDragMode = InteractionType_PrimaryEdit;

    if (hit.hit && hit.tag == nullptr)
    {
        mEditingPos = hit.worldPos;
        mIsEditing = true;
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.beginMacro("Set land normals");
    }

    return true;
}

bool CSVRender::TerrainVertexPaintMode::secondaryEditStartDrag(const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainVertexPaintMode::primarySelectStartDrag(const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
    mDragMode = InteractionType_PrimarySelect;
    if (!hit.hit || hit.tag != nullptr)
    {
        mDragMode = InteractionType_None;
        return false;
    }
    selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0);
    return true;
}

bool CSVRender::TerrainVertexPaintMode::secondarySelectStartDrag(const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
    mDragMode = InteractionType_SecondarySelect;
    if (!hit.hit || hit.tag != nullptr)
    {
        mDragMode = InteractionType_None;
        return false;
    }
    selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1);
    return true;
}

void CSVRender::TerrainVertexPaintMode::drag(const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    if (mDragMode == InteractionType_PrimaryEdit)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
        mTotalDiffY += diffY;
        if (mIsEditing)
            editVertexColourGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
    }

    if (mDragMode == InteractionType_PrimarySelect)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
        if (hit.hit && hit.tag == nullptr)
            selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0);
    }

    if (mDragMode == InteractionType_SecondarySelect)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
        if (hit.hit && hit.tag == nullptr)
            selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1);
    }
}

void CSVRender::TerrainVertexPaintMode::dragCompleted(const QPoint& pos)
{
    if (mDragMode == InteractionType_PrimaryEdit)
    {
        if (mIsEditing)
        {
            CSMDoc::Document& document = getWorldspaceWidget().getDocument();
            QUndoStack& undoStack = document.getUndoStack();
            undoStack.endMacro();
        }
        endVertexPaintEditing();
    }
    if (mDragMode == InteractionType_PrimarySelect || mDragMode == InteractionType_SecondarySelect)
    {
        mTerrainSelection->clearTemporarySelection();
    }
}

void CSVRender::TerrainVertexPaintMode::dragAborted()
{
    if (mIsEditing)
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.endMacro();
    } 
    endVertexPaintEditing();
    mDragMode = InteractionType_None;
}

void CSVRender::TerrainVertexPaintMode::dragWheel(int diff, double speedFactor) {}

void CSVRender::TerrainVertexPaintMode::endVertexPaintEditing()
{
    mIsEditing = false;
    mTerrainSelection->update();
}

void CSVRender::TerrainVertexPaintMode::editVertexColourGrid(
    const std::pair<int, int>& vertexCoords, bool dragOperation)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));

    std::string mCellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
    if (allowLandColourEditing(mCellId))
    {
    }

    std::pair<CSMWorld::CellCoordinates, bool> cellCoordinates_pair = CSMWorld::CellCoordinates::fromId(mCellId);

    int cellX = cellCoordinates_pair.first.getX();
    int cellY = cellCoordinates_pair.first.getY();

    int xHitInCell = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.first);
    int yHitInCell = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.second);
    if (xHitInCell < 0)
    {
        xHitInCell = xHitInCell + ESM::Land::LAND_SIZE;
        cellX = cellX - 1;
    }
    if (yHitInCell > 64)
    {
        yHitInCell = yHitInCell - ESM::Land::LAND_SIZE;
        cellY = cellY + 1;
    }

    mCellId = CSMWorld::CellCoordinates::generateId(cellX, cellY);
    if (allowLandColourEditing(mCellId))
    {
    }

    std::string iteratedCellId;

    int colourColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandColoursIndex);

    int r = std::max(static_cast<float>(mBrushSize) / 2.0f, 1.0f);

    if (mBrushShape == CSVWidget::BrushShape_Point)
    {
        CSMWorld::LandColoursColumn::DataType newTerrain
            = landTable.data(landTable.getModelIndex(mCellId, colourColumn))
                  .value<CSMWorld::LandColoursColumn::DataType>();

        if (allowLandColourEditing(mCellId))
        {
            alterColour(newTerrain, xHitInCell, yHitInCell, 0.0f);
            pushEditToCommand(newTerrain, document, landTable, mCellId);
        }
    }

    if (mBrushShape == CSVWidget::BrushShape_Square)
    {
        int upperLeftCellX = cellX - std::floor(r / ESM::Land::LAND_SIZE);
        int upperLeftCellY = cellY - std::floor(r / ESM::Land::LAND_SIZE);
        if (xHitInCell - (r % ESM::Land::LAND_SIZE) < 0)
            upperLeftCellX--;
        if (yHitInCell - (r % ESM::Land::LAND_SIZE) < 0)
            upperLeftCellY--;

        int lowerrightCellX = cellX + std::floor(r / ESM::Land::LAND_SIZE);
        int lowerrightCellY = cellY + std::floor(r / ESM::Land::LAND_SIZE);
        if (xHitInCell + (r % ESM::Land::LAND_SIZE) > ESM::Land::LAND_SIZE - 1)
            lowerrightCellX++;
        if (yHitInCell + (r % ESM::Land::LAND_SIZE) > ESM::Land::LAND_SIZE - 1)
            lowerrightCellY++;

        for (int i_cell = upperLeftCellX; i_cell <= lowerrightCellX; i_cell++)
        {
            for (int j_cell = upperLeftCellY; j_cell <= lowerrightCellY; j_cell++)
            {
                iteratedCellId = CSMWorld::CellCoordinates::generateId(i_cell, j_cell);
                if (allowLandColourEditing(iteratedCellId))
                {
                    CSMWorld::LandColoursColumn::DataType newTerrain
                        = landTable.data(landTable.getModelIndex(iteratedCellId, colourColumn))
                              .value<CSMWorld::LandColoursColumn::DataType>();
                    for (int i = 0; i < ESM::Land::LAND_SIZE; i++)
                    {
                        for (int j = 0; j < ESM::Land::LAND_SIZE; j++)
                        {

                            if (i_cell == cellX && j_cell == cellY && abs(i - xHitInCell) < r
                                && abs(j - yHitInCell) < r)
                            {
                                alterColour(newTerrain, i, j, 0.0f);
                            }
                            else
                            {
                                int distanceX(0);
                                int distanceY(0);
                                if (i_cell < cellX)
                                    distanceX = xHitInCell + ESM::Land::LAND_SIZE * abs(i_cell - cellX) - i;
                                if (j_cell < cellY)
                                    distanceY = yHitInCell + ESM::Land::LAND_SIZE * abs(j_cell - cellY) - j;
                                if (i_cell > cellX)
                                    distanceX = -xHitInCell + ESM::Land::LAND_SIZE * abs(i_cell - cellX) + i;
                                if (j_cell > cellY)
                                    distanceY = -yHitInCell + ESM::Land::LAND_SIZE * abs(j_cell - cellY) + j;
                                if (i_cell == cellX)
                                    distanceX = abs(i - xHitInCell);
                                if (j_cell == cellY)
                                    distanceY = abs(j - yHitInCell);
                                if (distanceX < r && distanceY < r)
                                    alterColour(newTerrain, i, j, 0.0f);
                            }
                        }
                    }
                    pushEditToCommand(newTerrain, document, landTable, iteratedCellId);
                }
            }
        }
    }

    if (mBrushShape == CSVWidget::BrushShape_Circle)
    {
        int upperLeftCellX = cellX - std::floor(r / ESM::Land::LAND_SIZE);
        int upperLeftCellY = cellY - std::floor(r / ESM::Land::LAND_SIZE);
        if (xHitInCell - (r % ESM::Land::LAND_SIZE) < 0)
            upperLeftCellX--;
        if (yHitInCell - (r % ESM::Land::LAND_SIZE) < 0)
            upperLeftCellY--;

        int lowerrightCellX = cellX + std::floor(r / ESM::Land::LAND_SIZE);
        int lowerrightCellY = cellY + std::floor(r / ESM::Land::LAND_SIZE);
        if (xHitInCell + (r % ESM::Land::LAND_SIZE) > ESM::Land::LAND_SIZE - 1)
            lowerrightCellX++;
        if (yHitInCell + (r % ESM::Land::LAND_SIZE) > ESM::Land::LAND_SIZE - 1)
            lowerrightCellY++;

        for (int i_cell = upperLeftCellX; i_cell <= lowerrightCellX; i_cell++)
        {
            for (int j_cell = upperLeftCellY; j_cell <= lowerrightCellY; j_cell++)
            {
                iteratedCellId = CSMWorld::CellCoordinates::generateId(i_cell, j_cell);
                if (allowLandColourEditing(iteratedCellId))
                {
                    CSMWorld::LandColoursColumn::DataType newTerrain
                        = landTable.data(landTable.getModelIndex(iteratedCellId, colourColumn))
                              .value<CSMWorld::LandColoursColumn::DataType>();
                    for (int i = 0; i < ESM::Land::LAND_SIZE; i++)
                    {
                        for (int j = 0; j < ESM::Land::LAND_SIZE; j++)
                        {
                            if (i_cell == cellX && j_cell == cellY && abs(i - xHitInCell) < r
                                && abs(j - yHitInCell) < r)
                            {
                                int distanceX = abs(i - xHitInCell);
                                int distanceY = abs(j - yHitInCell);
                                float distance = std::round(sqrt(pow(distanceX, 2) + pow(distanceY, 2)));
                                if (distance < r)
                                    alterColour(newTerrain, i, j, 0.0f);
                            }
                            else
                            {
                                int distanceX(0);
                                int distanceY(0);
                                if (i_cell < cellX)
                                    distanceX = xHitInCell + ESM::Land::LAND_SIZE * abs(i_cell - cellX) - i;
                                if (j_cell < cellY)
                                    distanceY = yHitInCell + ESM::Land::LAND_SIZE * abs(j_cell - cellY) - j;
                                if (i_cell > cellX)
                                    distanceX = -xHitInCell + ESM::Land::LAND_SIZE * abs(i_cell - cellX) + i;
                                if (j_cell > cellY)
                                    distanceY = -yHitInCell + ESM::Land::LAND_SIZE * abs(j_cell - cellY) + j;
                                if (i_cell == cellX)
                                    distanceX = abs(i - xHitInCell);
                                if (j_cell == cellY)
                                    distanceY = abs(j - yHitInCell);
                                float distance = std::round(sqrt(pow(distanceX, 2) + pow(distanceY, 2)));
                                if (distance < r)
                                    alterColour(newTerrain, i, j, 0.0f);
                            }
                        }
                    }
                    pushEditToCommand(newTerrain, document, landTable, iteratedCellId);
                }
            }
        }
    }
}

void CSVRender::TerrainVertexPaintMode::alterColour(
    CSMWorld::LandColoursColumn::DataType& landColorsNew, int inCellX, int inCellY, float alteredHeight, bool useTool)
{
    const int red = mVertexPaintEditToolColor.red();
    const int green = mVertexPaintEditToolColor.green();
    const int blue = mVertexPaintEditToolColor.blue();

    // TODO: handle different smoothing/blend types with different tools, right now this expects Replace
    landColorsNew[(inCellY * ESM::Land::LAND_SIZE + inCellX) * 3 + 0] = red;
    landColorsNew[(inCellY * ESM::Land::LAND_SIZE + inCellX) * 3 + 1] = green;
    landColorsNew[(inCellY * ESM::Land::LAND_SIZE + inCellX) * 3 + 2] = blue;
}

void CSVRender::TerrainVertexPaintMode::pushEditToCommand(const CSMWorld::LandColoursColumn::DataType& newLandColours,
    CSMDoc::Document& document, CSMWorld::IdTable& landTable, const std::string& cellId)
{
    QVariant changedLand;
    changedLand.setValue(newLandColours);

    QModelIndex index(
        landTable.getModelIndex(cellId, landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandColoursIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push(new CSMWorld::ModifyCommand(landTable, index, changedLand));

}

bool CSVRender::TerrainVertexPaintMode::isInCellSelection(int globalSelectionX, int globalSelectionY)
{
    if (CSVRender::PagedWorldspaceWidget* paged
        = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
    {
        std::pair<int, int> vertexCoords = std::make_pair(globalSelectionX, globalSelectionY);
        std::string cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
        return paged->getCellSelection().has(CSMWorld::CellCoordinates::fromId(cellId).first) && isLandLoaded(cellId);
    }
    return false;
}

void CSVRender::TerrainVertexPaintMode::handleSelection(
    int globalSelectionX, int globalSelectionY, std::vector<std::pair<int, int>>* selections)
{
    if (isInCellSelection(globalSelectionX, globalSelectionY))
        selections->emplace_back(globalSelectionX, globalSelectionY);
    else
    {
        int moduloX = globalSelectionX % (ESM::Land::LAND_SIZE - 1);
        int moduloY = globalSelectionY % (ESM::Land::LAND_SIZE - 1);
        bool xIsAtCellBorder = moduloX == 0;
        bool yIsAtCellBorder = moduloY == 0;
        if (!xIsAtCellBorder && !yIsAtCellBorder)
            return;
        int selectionX = globalSelectionX;
        int selectionY = globalSelectionY;

        /*
            The northern and eastern edges don't belong to the current cell.
            If the corresponding adjacent cell is not loaded, some special handling is necessary to select border
           vertices.
        */
        if (xIsAtCellBorder && yIsAtCellBorder)
        {
            /*
                Handle the NW, NE, and SE corner vertices.
                NW corner: (+1, -1) offset to reach current cell.
                NE corner: (-1, -1) offset to reach current cell.
                SE corner: (-1, +1) offset to reach current cell.
            */
            if (isInCellSelection(globalSelectionX - 1, globalSelectionY - 1)
                || isInCellSelection(globalSelectionX + 1, globalSelectionY - 1)
                || isInCellSelection(globalSelectionX - 1, globalSelectionY + 1))
            {
                selections->emplace_back(globalSelectionX, globalSelectionY);
            }
        }
        else if (xIsAtCellBorder)
        {
            selectionX--;
        }
        else if (yIsAtCellBorder)
        {
            selectionY--;
        }

        if (isInCellSelection(selectionX, selectionY))
            selections->emplace_back(globalSelectionX, globalSelectionY);
    }
}

void CSVRender::TerrainVertexPaintMode::selectTerrainShapes(
    const std::pair<int, int>& vertexCoords, unsigned char selectMode)
{
    int r = mBrushSize / 2;
    std::vector<std::pair<int, int>> selections;

    if (mBrushShape == CSVWidget::BrushShape_Point)
    {
        handleSelection(vertexCoords.first, vertexCoords.second, &selections);
    }

    if (mBrushShape == CSVWidget::BrushShape_Square)
    {
        for (int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for (int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                handleSelection(i, j, &selections);
            }
        }
    }

    if (mBrushShape == CSVWidget::BrushShape_Circle)
    {
        for (int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for (int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                int distanceX = abs(i - vertexCoords.first);
                int distanceY = abs(j - vertexCoords.second);
                float distance = sqrt(pow(distanceX, 2) + pow(distanceY, 2));

                // Using floating-point radius here to prevent selecting too few vertices.
                if (distance <= mBrushSize / 2.0f)
                    handleSelection(i, j, &selections);
            }
        }
    }

    std::string selectAction;

    if (selectMode == 0)
        selectAction = CSMPrefs::get()["3D Scene Editing"]["primary-select-action"].toString();
    else
        selectAction = CSMPrefs::get()["3D Scene Editing"]["secondary-select-action"].toString();

    if (selectAction == "Select only")
        mTerrainSelection->onlySelect(selections);
    else if (selectAction == "Add to selection")
        mTerrainSelection->addSelect(selections);
    else if (selectAction == "Remove from selection")
        mTerrainSelection->removeSelect(selections);
    else if (selectAction == "Invert selection")
        mTerrainSelection->toggleSelect(selections);
}

bool CSVRender::TerrainVertexPaintMode::noCell(const std::string& cellId)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    const CSMWorld::IdCollection<CSMWorld::Cell>& cellCollection = document.getData().getCells();
    return cellCollection.searchId(ESM::RefId::stringRefId(cellId)) == -1;
}

bool CSVRender::TerrainVertexPaintMode::noLand(const std::string& cellId)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    const CSMWorld::IdCollection<CSMWorld::Land>& landCollection = document.getData().getLand();
    return landCollection.searchId(ESM::RefId::stringRefId(cellId)) == -1;
}

bool CSVRender::TerrainVertexPaintMode::noLandLoaded(const std::string& cellId)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    const CSMWorld::IdCollection<CSMWorld::Land>& landCollection = document.getData().getLand();
    return !landCollection.getRecord(ESM::RefId::stringRefId(cellId)).get().isDataLoaded(ESM::Land::DATA_VNML);
}

bool CSVRender::TerrainVertexPaintMode::isLandLoaded(const std::string& cellId)
{
    if (!noCell(cellId) && !noLand(cellId) && !noLandLoaded(cellId))
        return true;
    return false;
}

void CSVRender::TerrainVertexPaintMode::createNewLandData(const CSMWorld::CellCoordinates& cellCoords)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_LandTextures));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    int landnormalsColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex);

    float defaultHeight = 0.f;
    int averageDivider = 0;
    CSMWorld::CellCoordinates cellLeftCoords = cellCoords.move(-1, 0);
    CSMWorld::CellCoordinates cellRightCoords = cellCoords.move(1, 0);
    CSMWorld::CellCoordinates cellUpCoords = cellCoords.move(0, -1);
    CSMWorld::CellCoordinates cellDownCoords = cellCoords.move(0, 1);

    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    std::string cellLeftId = CSMWorld::CellCoordinates::generateId(cellLeftCoords.getX(), cellLeftCoords.getY());
    std::string cellRightId = CSMWorld::CellCoordinates::generateId(cellRightCoords.getX(), cellRightCoords.getY());
    std::string cellUpId = CSMWorld::CellCoordinates::generateId(cellUpCoords.getX(), cellUpCoords.getY());
    std::string cellDownId = CSMWorld::CellCoordinates::generateId(cellDownCoords.getX(), cellDownCoords.getY());

    float leftCellSampleHeight = 0.0f;
    float rightCellSampleHeight = 0.0f;
    float upCellSampleHeight = 0.0f;
    float downCellSampleHeight = 0.0f;

    const CSMWorld::LandHeightsColumn::DataType landShapePointer
        = landTable.data(landTable.getModelIndex(cellId, landshapeColumn))
              .value<CSMWorld::LandHeightsColumn::DataType>();
    const CSMWorld::LandNormalsColumn::DataType landNormalsPointer
        = landTable.data(landTable.getModelIndex(cellId, landnormalsColumn))
              .value<CSMWorld::LandNormalsColumn::DataType>();
    CSMWorld::LandHeightsColumn::DataType landShapeNew(landShapePointer);
    CSMWorld::LandNormalsColumn::DataType landNormalsNew(landNormalsPointer);

    if (CSVRender::PagedWorldspaceWidget* paged
        = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
    {
        if (isLandLoaded(cellLeftId))
        {
            const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer
                = landTable.data(landTable.getModelIndex(cellLeftId, landshapeColumn))
                      .value<CSMWorld::LandHeightsColumn::DataType>();

            ++averageDivider;
            leftCellSampleHeight
                = landLeftShapePointer[(ESM::Land::LAND_SIZE / 2) * ESM::Land::LAND_SIZE + ESM::Land::LAND_SIZE - 1];
            if (paged->getCellAlteredHeight(cellLeftCoords, ESM::Land::LAND_SIZE - 1, ESM::Land::LAND_SIZE / 2))
                leftCellSampleHeight
                    += *paged->getCellAlteredHeight(cellLeftCoords, ESM::Land::LAND_SIZE - 1, ESM::Land::LAND_SIZE / 2);
        }
        if (isLandLoaded(cellRightId))
        {
            const CSMWorld::LandHeightsColumn::DataType landRightShapePointer
                = landTable.data(landTable.getModelIndex(cellRightId, landshapeColumn))
                      .value<CSMWorld::LandHeightsColumn::DataType>();

            ++averageDivider;
            rightCellSampleHeight = landRightShapePointer[(ESM::Land::LAND_SIZE / 2) * ESM::Land::LAND_SIZE];
            if (paged->getCellAlteredHeight(cellRightCoords, 0, ESM::Land::LAND_SIZE / 2))
                rightCellSampleHeight += *paged->getCellAlteredHeight(cellRightCoords, 0, ESM::Land::LAND_SIZE / 2);
        }
        if (isLandLoaded(cellUpId))
        {
            const CSMWorld::LandHeightsColumn::DataType landUpShapePointer
                = landTable.data(landTable.getModelIndex(cellUpId, landshapeColumn))
                      .value<CSMWorld::LandHeightsColumn::DataType>();

            ++averageDivider;
            upCellSampleHeight
                = landUpShapePointer[(ESM::Land::LAND_SIZE - 1) * ESM::Land::LAND_SIZE + (ESM::Land::LAND_SIZE / 2)];
            if (paged->getCellAlteredHeight(cellUpCoords, ESM::Land::LAND_SIZE / 2, ESM::Land::LAND_SIZE - 1))
                upCellSampleHeight
                    += *paged->getCellAlteredHeight(cellUpCoords, ESM::Land::LAND_SIZE / 2, ESM::Land::LAND_SIZE - 1);
        }
        if (isLandLoaded(cellDownId))
        {
            const CSMWorld::LandHeightsColumn::DataType landDownShapePointer
                = landTable.data(landTable.getModelIndex(cellDownId, landshapeColumn))
                      .value<CSMWorld::LandHeightsColumn::DataType>();

            ++averageDivider;
            downCellSampleHeight = landDownShapePointer[ESM::Land::LAND_SIZE / 2];
            if (paged->getCellAlteredHeight(cellDownCoords, ESM::Land::LAND_SIZE / 2, 0))
                downCellSampleHeight += *paged->getCellAlteredHeight(cellDownCoords, ESM::Land::LAND_SIZE / 2, 0);
        }
    }
    if (averageDivider > 0)
        defaultHeight = (leftCellSampleHeight + rightCellSampleHeight + upCellSampleHeight + downCellSampleHeight)
            / averageDivider;

    for (int i = 0; i < ESM::Land::LAND_SIZE; ++i)
    {
        for (int j = 0; j < ESM::Land::LAND_SIZE; ++j)
        {
            landShapeNew[j * ESM::Land::LAND_SIZE + i] = defaultHeight;
            landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 0] = 0;
            landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 1] = 0;
            landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 2] = 127;
        }
    }
    QVariant changedShape;
    changedShape.setValue(landShapeNew);
    QVariant changedNormals;
    changedNormals.setValue(landNormalsNew);
    QModelIndex indexShape(
        landTable.getModelIndex(cellId, landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex)));
    QModelIndex indexNormal(
        landTable.getModelIndex(cellId, landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex)));
    document.getUndoStack().push(new CSMWorld::TouchLandCommand(landTable, ltexTable, cellId));
    document.getUndoStack().push(new CSMWorld::ModifyCommand(landTable, indexShape, changedShape));
    document.getUndoStack().push(new CSMWorld::ModifyCommand(landTable, indexNormal, changedNormals));
}

bool CSVRender::TerrainVertexPaintMode::allowLandColourEditing(const std::string& cellId, bool useTool)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTree& cellTable
        = dynamic_cast<CSMWorld::IdTree&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Cells));

    if (noCell(cellId))
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode == "Discard")
            return false;

        if (mode == "Create cell and land, then edit" && useTool)
        {
            auto createCommand = std::make_unique<CSMWorld::CreateCommand>(cellTable, cellId);
            int parentIndex = cellTable.findColumnIndex(CSMWorld::Columns::ColumnId_Cell);
            int index = cellTable.findNestedColumnIndex(parentIndex, CSMWorld::Columns::ColumnId_Interior);
            createCommand->addNestedValue(parentIndex, index, false);
            document.getUndoStack().push(createCommand.release());

            if (CSVRender::PagedWorldspaceWidget* paged
                = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
            {
                CSMWorld::CellSelection selection = paged->getCellSelection();
                selection.add(CSMWorld::CellCoordinates::fromId(cellId).first);
                paged->setCellSelection(selection);
            }
        }
    }
    else if (CSVRender::PagedWorldspaceWidget* paged
        = dynamic_cast<CSVRender::PagedWorldspaceWidget*>(&getWorldspaceWidget()))
    {
        CSMWorld::CellSelection selection = paged->getCellSelection();
        if (!selection.has(CSMWorld::CellCoordinates::fromId(cellId).first))
        {
            // target cell exists, but is not shown
            std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-visible-landedit"].toString();

            if (mode == "Discard")
                return false;

            if (mode == "Show cell and edit" && useTool)
            {
                selection.add(CSMWorld::CellCoordinates::fromId(cellId).first);
                paged->setCellSelection(selection);
            }
        }
    }

    if (noLand(cellId))
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode == "Discard")
            return false;

        if (mode == "Create cell and land, then edit" && useTool)
        {
            document.getUndoStack().push(new CSMWorld::CreateCommand(landTable, cellId));
            createNewLandData(CSMWorld::CellCoordinates::fromId(cellId).first);
        }
    }
    else if (noLandLoaded(cellId))
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        if (mode == "Discard")
            return false;

        if (mode == "Create cell and land, then edit" && useTool)
        {
            createNewLandData(CSMWorld::CellCoordinates::fromId(cellId).first);
        }
    }

    if (useTool && (noCell(cellId) || noLand(cellId) || noLandLoaded(cellId)))
    {
        Log(Debug::Warning) << "Land creation failed at cell id: " << cellId;
        return false;
    }
    return true;
}

void CSVRender::TerrainVertexPaintMode::dragMoveEvent(QDragMoveEvent* event) {}

void CSVRender::TerrainVertexPaintMode::mouseMoveEvent(QMouseEvent* event)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(event->pos(), getInteractionMask());
    if (hit.hit && mBrushDraw)
        mBrushDraw->update(hit.worldPos, mBrushSize, mBrushShape);
    if (!hit.hit && mBrushDraw)
        mBrushDraw->hide();
}

std::shared_ptr<CSVRender::TerrainSelection> CSVRender::TerrainVertexPaintMode::getTerrainSelection()
{
    return mTerrainSelection;
}

void CSVRender::TerrainVertexPaintMode::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
}

void CSVRender::TerrainVertexPaintMode::setBrushShape(CSVWidget::BrushShape brushShape)
{
    mBrushShape = brushShape;
}

void CSVRender::TerrainVertexPaintMode::setVertexPaintEditTool(int shapeEditTool)
{
    mVertexPaintEditTool = shapeEditTool;
}

void CSVRender::TerrainVertexPaintMode::setVertexPaintColor(const QColor& color)
{
    mVertexPaintEditToolColor = color;
}

CSVRender::PagedWorldspaceWidget& CSVRender::TerrainVertexPaintMode::getPagedWorldspaceWidget()
{
    return dynamic_cast<PagedWorldspaceWidget&>(getWorldspaceWidget());
}
