#include "terrainvertexpaintmode.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include <QComboBox>
#include <QDropEvent>
#include <QEvent>
#include <QIcon>

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/prefs/state.hpp>
#include <apps/opencs/model/world/cellselection.hpp>
#include <apps/opencs/model/world/columnimp.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idtable.hpp>
#include <apps/opencs/model/world/land.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/widget/brushshapes.hpp>
#include <apps/opencs/view/widget/scenetool.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadland.hpp>

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolvertexpaintbrush.hpp"

#include "../../model/world/commands.hpp"
#include "../../model/world/idtree.hpp"

#include "brushdraw.hpp"
#include "commands.hpp"
#include "editmode.hpp"
#include "mask.hpp"
#include "pagedworldspacewidget.hpp"
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
        mRestoreMode = true;
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.beginMacro("Restore land vertex colour");
        editVertexColourGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
        undoStack.endMacro();
    }
    mRestoreMode = false;
}

void CSVRender::TerrainVertexPaintMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    if (hit.hit && hit.tag == nullptr)
    {
        mRestoreMode = false;
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.beginMacro("Paint land vertex colour");
        editVertexColourGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
        undoStack.endMacro();
    }
}

bool CSVRender::TerrainVertexPaintMode::primaryEditStartDrag(const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());

    mDragMode = InteractionType_PrimaryEdit;

    if (hit.hit && hit.tag == nullptr)
    {
        mIsEditing = true;
        mRestoreMode = true;
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.beginMacro("Restore land vertex colours");
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

    if (hit.hit && hit.tag == nullptr)
    {
        mIsEditing = true;
        mRestoreMode = false;
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();
        undoStack.beginMacro("Paint land vertex colours");
    }

    return true;
}

void CSVRender::TerrainVertexPaintMode::drag(const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    if (mDragMode == InteractionType_PrimarySelect || mDragMode == InteractionType_PrimaryEdit)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());
        if (mIsEditing)
            editVertexColourGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
    }
}

void CSVRender::TerrainVertexPaintMode::dragCompleted(const QPoint& pos)
{
    if (mDragMode == InteractionType_PrimarySelect || mDragMode == InteractionType_PrimaryEdit)
    {
        if (mIsEditing)
        {
            CSMDoc::Document& document = getWorldspaceWidget().getDocument();
            QUndoStack& undoStack = document.getUndoStack();
            undoStack.endMacro();
        }
        endVertexPaintEditing();
        mRestoreMode = false;
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
    mRestoreMode = false;
}

void CSVRender::TerrainVertexPaintMode::dragWheel(int diff, double speedFactor) {}

void CSVRender::TerrainVertexPaintMode::endVertexPaintEditing()
{
    mIsEditing = false;
}

void CSVRender::TerrainVertexPaintMode::editVertexColourGrid(
    const std::pair<int, int>& vertexCoords, bool dragOperation)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));

    std::string mCellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);

    if (!allowLandColourEditing(mCellId))
        return;

    std::pair<CSMWorld::CellCoordinates, bool> cellCoordinatesPair = CSMWorld::CellCoordinates::fromId(mCellId);

    int cellX = cellCoordinatesPair.first.getX();
    int cellY = cellCoordinatesPair.first.getY();

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
    if (!allowLandColourEditing(mCellId))
        return;

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
            alterColour(newTerrain, xHitInCell, yHitInCell);
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

        for (int iCell = upperLeftCellX; iCell <= lowerrightCellX; iCell++)
        {
            for (int jCell = upperLeftCellY; jCell <= lowerrightCellY; jCell++)
            {
                iteratedCellId = CSMWorld::CellCoordinates::generateId(iCell, jCell);
                if (allowLandColourEditing(iteratedCellId))
                {
                    CSMWorld::LandColoursColumn::DataType newTerrain
                        = landTable.data(landTable.getModelIndex(iteratedCellId, colourColumn))
                              .value<CSMWorld::LandColoursColumn::DataType>();
                    for (int i = 0; i < ESM::Land::LAND_SIZE; i++)
                    {
                        for (int j = 0; j < ESM::Land::LAND_SIZE; j++)
                        {
                            osg::Vec2f relativeCoords(0.0, 0.0);
                            if (iCell < cellX)
                                relativeCoords.x() = xHitInCell + ESM::Land::LAND_SIZE * abs(iCell - cellX) - i;
                            else if (iCell > cellX)
                                relativeCoords.x() = -xHitInCell + ESM::Land::LAND_SIZE * abs(iCell - cellX) + i;
                            else
                                relativeCoords.x() = abs(i - xHitInCell);
                            if (jCell < cellY)
                                relativeCoords.y() = yHitInCell + ESM::Land::LAND_SIZE * abs(jCell - cellY) - j;
                            else if (jCell > cellY)
                                relativeCoords.y() = -yHitInCell + ESM::Land::LAND_SIZE * abs(jCell - cellY) + j;
                            else
                                relativeCoords.y() = abs(j - yHitInCell);
                            if (relativeCoords.x() < r && relativeCoords.y() < r)
                                alterColour(newTerrain, i, j);
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

        for (int iCell = upperLeftCellX; iCell <= lowerrightCellX; iCell++)
        {
            for (int jCell = upperLeftCellY; jCell <= lowerrightCellY; jCell++)
            {
                iteratedCellId = CSMWorld::CellCoordinates::generateId(iCell, jCell);
                if (allowLandColourEditing(iteratedCellId))
                {
                    CSMWorld::LandColoursColumn::DataType newTerrain
                        = landTable.data(landTable.getModelIndex(iteratedCellId, colourColumn))
                              .value<CSMWorld::LandColoursColumn::DataType>();
                    for (int i = 0; i < ESM::Land::LAND_SIZE; i++)
                    {
                        for (int j = 0; j < ESM::Land::LAND_SIZE; j++)
                        {
                            osg::Vec2f relativeCoords(0.0, 0.0);
                            if (iCell < cellX)
                                relativeCoords.x() = xHitInCell + ESM::Land::LAND_SIZE * abs(iCell - cellX) - i;
                            else if (iCell > cellX)
                                relativeCoords.x() = -xHitInCell + ESM::Land::LAND_SIZE * abs(iCell - cellX) + i;
                            else
                                relativeCoords.x() = abs(i - xHitInCell);
                            if (jCell < cellY)
                                relativeCoords.y() = yHitInCell + ESM::Land::LAND_SIZE * abs(jCell - cellY) - j;
                            else if (jCell > cellY)
                                relativeCoords.y() = -yHitInCell + ESM::Land::LAND_SIZE * abs(jCell - cellY) + j;
                            else
                                relativeCoords.y() = abs(j - yHitInCell);
                            if (relativeCoords.length() < r)
                                alterColour(newTerrain, i, j);
                        }
                    }
                    pushEditToCommand(newTerrain, document, landTable, iteratedCellId);
                }
            }
        }
    }
}

void CSVRender::TerrainVertexPaintMode::alterColour(
    CSMWorld::LandColoursColumn::DataType& landColorsNew, int inCellX, int inCellY, bool useTool)
{
    const int red = mRestoreMode ? 255 : mVertexPaintEditToolColor.red();
    const int green = mRestoreMode ? 255 : mVertexPaintEditToolColor.green();
    const int blue = mRestoreMode ? 255 : mVertexPaintEditToolColor.blue();

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

bool CSVRender::TerrainVertexPaintMode::allowLandColourEditing(const std::string& cellId, bool useTool)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable
        = dynamic_cast<CSMWorld::IdTable&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTree& cellTable
        = dynamic_cast<CSMWorld::IdTree&>(*document.getData().getTableModel(CSMWorld::UniversalId::Type_Cells));

    const ESM::RefId cellRefId = ESM::RefId::stringRefId(cellId);
    const bool noCell = document.getData().getCells().searchId(cellRefId) == -1;
    const bool noLand = document.getData().getLand().searchId(cellRefId) == -1;

    if (noCell)
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode == "Discard")
            return false;

        if (useTool && mode == "Create cell and land, then edit")
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

            if (useTool && mode == "Show cell and edit")
            {
                selection.add(CSMWorld::CellCoordinates::fromId(cellId).first);
                paged->setCellSelection(selection);
            }
        }
    }

    if (noLand)
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode == "Discard")
            return false;

        if (mode == "Create cell and land, then edit")
        {
            document.getUndoStack().push(new CSMWorld::CreateCommand(landTable, cellId));
        }
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
