#include "terrainshapemode.hpp"

#include <string>
#include <sstream>
#include <memory>

#include <QWidget>
#include <QIcon>
#include <QEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDrag>

#include <osg/Group>

#include <components/esm/loadland.hpp>

#include "../widget/modebutton.hpp"
#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolshapebrush.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/land.hpp"
#include "../../model/world/resourcetable.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/universalid.hpp"

#include "editmode.hpp"
#include "pagedworldspacewidget.hpp"
#include "mask.hpp"
#include "object.hpp" // Something small needed regarding pointers from here ()
#include "terrainselection.hpp"
#include "worldspacewidget.hpp"

CSVRender::TerrainShapeMode::TerrainShapeMode (WorldspaceWidget *worldspaceWidget, osg::Group* parentNode, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-shape"}, Mask_Terrain | Mask_Reference, "Terrain land editing", parent),
    mBrushSize(0),
    mBrushShape(0),
    mShapeBrushScenetool(0),
    mDragMode(InteractionType_None),
    mParentNode(parentNode),
    mIsEditing(false)
{
    resetAlteredHeightMap();
}

void CSVRender::TerrainShapeMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    if (!mTerrainShapeSelection)
    {
        mTerrainShapeSelection.reset(new TerrainSelection(mParentNode, &getWorldspaceWidget(), TerrainSelectionType::Shape));
    }

    if(!mShapeBrushScenetool)
    {
        mShapeBrushScenetool = new CSVWidget::SceneToolShapeBrush (toolbar, "scenetoolshapebrush", getWorldspaceWidget().getDocument());
        connect(mShapeBrushScenetool, SIGNAL (clicked()), mShapeBrushScenetool, SLOT (activate()));
        connect(mShapeBrushScenetool->mShapeBrushWindow, SIGNAL(passBrushSize(int)), this, SLOT(setBrushSize(int)));
        connect(mShapeBrushScenetool->mShapeBrushWindow, SIGNAL(passBrushShape(int)), this, SLOT(setBrushShape(int)));
        connect(mShapeBrushScenetool->mShapeBrushWindow->mSizeSliders->mBrushSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrushSize(int)));
    }

    EditMode::activate(toolbar);
    toolbar->addTool (mShapeBrushScenetool);
}

void CSVRender::TerrainShapeMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    if(mShapeBrushScenetool)
    {
        toolbar->removeTool (mShapeBrushScenetool);
        delete mShapeBrushScenetool;
        mShapeBrushScenetool = 0;
    }
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainShapeMode::primaryEditPressed(const WorldspaceHitResult& hit)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    QUndoStack& undoStack = document.getUndoStack();

    if (hit.hit && hit.tag == 0)
    {
        undoStack.beginMacro ("Edit shape records");
        if(allowLandShapeEditing(mCellId)==true)
        {
            //undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, mCellId));
            editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), false);
        }
        undoStack.endMacro();
    }
}

void CSVRender::TerrainShapeMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    if(hit.hit && hit.tag == 0)
    {
        selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0, false);
    }
}

void CSVRender::TerrainShapeMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
    if(hit.hit && hit.tag == 0)
    {
        selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1, false);
    }
}

bool CSVRender::TerrainShapeMode::primaryEditStartDrag (const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    QUndoStack& undoStack = document.getUndoStack();

    mDragMode = InteractionType_PrimaryEdit;

    if (hit.hit && hit.tag == 0)
    {
        undoStack.beginMacro ("Edit shape records");
        mIsEditing = true;
        if(allowLandShapeEditing(mCellId)==true)
        {
            //undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, mCellId));
            editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
        }
    }

    return true;
}

bool CSVRender::TerrainShapeMode::secondaryEditStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainShapeMode::primarySelectStartDrag (const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
    mDragMode = InteractionType_PrimarySelect;
    if (!hit.hit || hit.tag != 0)
    {
        mDragMode = InteractionType_None;
        return false;
    }
    selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0, true);
    return false;
}

bool CSVRender::TerrainShapeMode::secondarySelectStartDrag (const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
    mDragMode = InteractionType_SecondarySelect;
    if (!hit.hit || hit.tag != 0)
    {
        mDragMode = InteractionType_None;
        return false;
    }
    selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1, true);
    return false;
}

void CSVRender::TerrainShapeMode::drag (const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    if (mDragMode == InteractionType_PrimaryEdit)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
        std::string cellId = getWorldspaceWidget().getCellId (hit.worldPos);
        getWorldspaceWidget().setCellBeingEdited(CSMWorld::CellCoordinates::fromId (cellId).first);
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();

        if (hit.hit && hit.tag == 0)
        {
            editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
        }
    }

    if (mDragMode == InteractionType_PrimarySelect)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
        if (hit.hit && hit.tag == 0) selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0, true);
    }

    if (mDragMode == InteractionType_SecondarySelect)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
        if (hit.hit && hit.tag == 0) selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1, true);
    }
}

void CSVRender::TerrainShapeMode::dragCompleted(const QPoint& pos)
{
    if (mDragMode == InteractionType_PrimaryEdit)
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        QUndoStack& undoStack = document.getUndoStack();

        getWorldspaceWidget().discardAllBeingEdited();
        if (mIsEditing == true)
        {
            undoStack.endMacro();
            mIsEditing = false;
        }
        resetAlteredHeightMap();
        if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            paged->resetAllAlteredHeights();
        }
    }
}

void CSVRender::TerrainShapeMode::dragAborted()
{
}

void CSVRender::TerrainShapeMode::dragWheel (int diff, double speedFactor)
{
}

void CSVRender::TerrainShapeMode::resetAlteredHeightMap()
{
    for (int x = 0; x < ESM::Land::LAND_SIZE; ++x)
    {
        for (int y = 0; y < ESM::Land::LAND_SIZE; ++y)
        {
            mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] = 0.0f;
        }
    }
}

void CSVRender::TerrainShapeMode::editTerrainShapeGrid(std::pair<int, int> vertexCoords, bool dragOperation)
{
    int r = mBrushSize / 2;

    std::string cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
    getWorldspaceWidget().setCellBeingEdited(CSMWorld::CellCoordinates::fromId(cellId).first);

    if (mBrushShape == 0)
    {
        int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.first);
        int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.second);
        if (x >= 0 && x < 65 && y >= 0 && y < 65)
        {
            mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] = mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] + 8.0f;
        }
    }

    if (mBrushShape == 1)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(i);
                int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(j);
                if (x >= 0 && x < 65 && y >= 0 && y < 65)
                {
                    mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] = mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] + 8.0f;
                }
            }
        }
    }

    if (mBrushShape == 2)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                int distanceX = abs(i - vertexCoords.first);
                int distanceY = abs(j - vertexCoords.second);
                int distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(i);
                int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(j);
                if (x >= 0 && x < 65 && y >= 0 && y < 65)
                {
                    if (distance < r) mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] = mAlteredHeightMap[y*ESM::Land::LAND_SIZE + x] + 8.0f;
                }
            }
        }
    }

    if (mBrushShape == 3)
    {
        if(!mCustomBrushShape.empty())
        {
            for(auto const& value: mCustomBrushShape)
            {
            }
        }
    }

    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        paged->setCellAlteredHeights(CSMWorld::CellCoordinates::fromId(cellId).first, mAlteredHeightMap);
    }
}

void CSVRender::TerrainShapeMode::selectTerrainShapes(std::pair<int, int> vertexCoords, unsigned char selectMode, bool dragOperation)
{
    int r = mBrushSize / 2;
    std::vector<std::pair<int, int>> selections;

    if (mBrushShape == 0)
    {
        selections.emplace_back(vertexCoords);
    }

    if (mBrushShape == 1)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                selections.emplace_back(std::make_pair(i, j));
            }
        }
    }

    if (mBrushShape == 2)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                int distanceX = abs(i - vertexCoords.first);
                int distanceY = abs(j - vertexCoords.second);
                int distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                if (distance < r) selections.emplace_back(std::make_pair(i, j));
            }
        }
    }

    if (mBrushShape == 3)
    {
        if(!mCustomBrushShape.empty())
        {
            for(auto const& value: mCustomBrushShape)
            {
                selections.emplace_back(std::make_pair(vertexCoords.first + value.first, vertexCoords.second + value.second));
            }
        }
    }

    if(selectMode == 0) mTerrainShapeSelection->onlySelect(selections);
    if(selectMode == 1) mTerrainShapeSelection->toggleSelect(selections, dragOperation);

}

/*void CSVRender::TerrainShapeMode::pushEditToCommand(CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, std::string cellId)
{
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandTexturesIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
    undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, cellId));
}*/

bool CSVRender::TerrainShapeMode::allowLandShapeEditing(std::string cellId)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTree& cellTable = dynamic_cast<CSMWorld::IdTree&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    bool noCell = document.getData().getCells().searchId (cellId)==-1;
    bool noLand = document.getData().getLand().searchId (cellId)==-1;

    if (noCell)
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode=="Discard")
            return false;

        if (mode=="Create cell and land, then edit")
        {
            std::unique_ptr<CSMWorld::CreateCommand> createCommand (
                new CSMWorld::CreateCommand (cellTable, cellId));
            int parentIndex = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
            int index = cellTable.findNestedColumnIndex (parentIndex, CSMWorld::Columns::ColumnId_Interior);
            createCommand->addNestedValue (parentIndex, index, false);
            document.getUndoStack().push (createCommand.release());

            if (CSVRender::PagedWorldspaceWidget *paged =
                dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
            {
                CSMWorld::CellSelection selection = paged->getCellSelection();
                selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                paged->setCellSelection (selection);
            }
        }
    }
    else if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        CSMWorld::CellSelection selection = paged->getCellSelection();
        if (!selection.has (CSMWorld::CellCoordinates::fromId (cellId).first))
        {
            // target cell exists, but is not shown
            std::string mode =
                CSMPrefs::get()["3D Scene Editing"]["outside-visible-landedit"].toString();

            if (mode=="Discard")
                return false;

            if (mode=="Show cell and edit")
            {
                selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                paged->setCellSelection (selection);
            }
        }
    }

    if (noLand)
    {
        std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-landedit"].toString();

        // target cell does not exist
        if (mode=="Discard")
            return false;

        if (mode=="Create cell and land, then edit")
        {
            document.getUndoStack().push (new CSMWorld::CreateCommand (landTable, cellId));
        }
    }

    return true;
}

void CSVRender::TerrainShapeMode::dragMoveEvent (QDragMoveEvent *event)
{
}

void CSVRender::TerrainShapeMode::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
}

void CSVRender::TerrainShapeMode::setBrushShape(int brushShape)
{
    mBrushShape = brushShape;

    //Set custom brush shape
    if (mBrushShape == 3 && !mTerrainShapeSelection->getTerrainSelection().empty())
    {
        auto terrainSelection = mTerrainShapeSelection->getTerrainSelection();
        int selectionCenterX = 0;
        int selectionCenterY = 0;
        int selectionAmount = 0;

        for(auto const& value: terrainSelection)
        {
            selectionCenterX = selectionCenterX + value.first;
            selectionCenterY = selectionCenterY + value.second;
            ++selectionAmount;
        }
        selectionCenterX = selectionCenterX / selectionAmount;
        selectionCenterY = selectionCenterY / selectionAmount;

        mCustomBrushShape.clear();
        std::pair<int, int> differentialPos {};
        for(auto const& value: terrainSelection)
        {
            differentialPos.first = value.first - selectionCenterX;
            differentialPos.second = value.second - selectionCenterY;
            mCustomBrushShape.push_back(differentialPos);
        }
    }
}

CSVRender::PagedWorldspaceWidget& CSVRender::TerrainShapeMode::getPagedWorldspaceWidget()
{
    return dynamic_cast<PagedWorldspaceWidget&>(getWorldspaceWidget());
}
