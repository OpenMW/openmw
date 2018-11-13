#include "terraintexturemode.hpp"

#include <string>
#include <sstream>

#include <QWidget>
#include <QIcon>
#include <QEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDrag>

#include <components/esm/loadland.hpp>

#include "../widget/modebutton.hpp"
#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetooltexturebrush.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/land.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/resourcetable.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/universalid.hpp"

#include "editmode.hpp"
#include "pagedworldspacewidget.hpp"
#include "mask.hpp"
#include "object.hpp" // Something small needed regarding pointers from here ()
#include "worldspacewidget.hpp"

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent),
    mBrushTexture("L0#0"),
    mBrushSize(0),
    mBrushShape(0),
    mTextureBrushScenetool(0)
{
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    if(!mTextureBrushScenetool)
    {
        mTextureBrushScenetool = new CSVWidget::SceneToolTextureBrush (toolbar, "scenetooltexturebrush", getWorldspaceWidget().getDocument());
        connect(mTextureBrushScenetool, SIGNAL (clicked()), mTextureBrushScenetool, SLOT (activate()));
        connect(mTextureBrushScenetool->mTextureBrushWindow, SIGNAL(passBrushSize(int)), this, SLOT(setBrushSize(int)));
        connect(mTextureBrushScenetool->mTextureBrushWindow, SIGNAL(passBrushShape(int)), this, SLOT(setBrushShape(int)));
        connect(mTextureBrushScenetool->mTextureBrushWindow->mSizeSliders->mBrushSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrushSize(int)));
        connect(mTextureBrushScenetool, SIGNAL(passTextureId(std::string)), this, SLOT(setBrushTexture(std::string)));
        connect(mTextureBrushScenetool->mTextureBrushWindow, SIGNAL(passTextureId(std::string)), this, SLOT(setBrushTexture(std::string)));

        connect(mTextureBrushScenetool, SIGNAL(passEvent(QDropEvent*)), this, SLOT(handleDropEvent(QDropEvent*)));
        connect(this, SIGNAL(passBrushTexture(std::string)), mTextureBrushScenetool->mTextureBrushWindow, SLOT(setBrushTexture(std::string)));
        connect(this, SIGNAL(passBrushTexture(std::string)), mTextureBrushScenetool, SLOT(updateBrushHistory(std::string)));
    }

    EditMode::activate(toolbar);
    toolbar->addTool (mTextureBrushScenetool);
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    if(mTextureBrushScenetool)
    {
        toolbar->removeTool (mTextureBrushScenetool);
        delete mTextureBrushScenetool;
        mTextureBrushScenetool = 0;
    }
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primaryEditPressed(const WorldspaceHitResult& hit) // Apply changes here
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    QUndoStack& undoStack = document.getUndoStack();
    CSMWorld::IdCollection<CSMWorld::LandTexture>& landtexturesCollection = document.getData().getLandTextures();
    int index = landtexturesCollection.searchId(mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted() && hit.hit == true)
    {
        undoStack.beginMacro ("Edit texture records");
        if(allowLandTextureEditing(mCellId)==true)
        {
            undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, mCellId));
            editTerrainTextureGrid(hit);
        }
        undoStack.endMacro();
    }
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
}

bool CSVRender::TerrainTextureMode::primaryEditStartDrag (const QPoint& pos)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::IdCollection<CSMWorld::LandTexture>& landtexturesCollection = document.getData().getLandTextures();
    int index = landtexturesCollection.searchId(mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted())
    {
        undoStack.beginMacro ("Edit texture records");
        if(allowLandTextureEditing(mCellId)==true && hit.hit == true)
        {
            undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, mCellId));
            editTerrainTextureGrid(hit);
        }
    }

    return true;
}

bool CSVRender::TerrainTextureMode::secondaryEditStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::primarySelectStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::secondarySelectStartDrag (const QPoint& pos)
{
    return false;
}

void CSVRender::TerrainTextureMode::drag (const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();

    CSMWorld::IdCollection<CSMWorld::LandTexture>& landtexturesCollection = document.getData().getLandTextures();
    int index = landtexturesCollection.searchId(mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted() && hit.hit == true)
    {
        editTerrainTextureGrid(hit);
    }
}

void CSVRender::TerrainTextureMode::dragCompleted(const QPoint& pos) {
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::IdCollection<CSMWorld::LandTexture>& landtexturesCollection = document.getData().getLandTextures();
    int index = landtexturesCollection.searchId(mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted())
    {
         undoStack.endMacro();
    }
}

void CSVRender::TerrainTextureMode::dragAborted() {
}

void CSVRender::TerrainTextureMode::dragWheel (int diff, double speedFactor) {}

void CSVRender::TerrainTextureMode::handleDropEvent (QDropEvent *event) {
  const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

  if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
      return;

  if (mime->holdsType (CSMWorld::UniversalId::Type_LandTexture))
  {
      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          mBrushTexture = uid.getId();
          emit passBrushTexture(mBrushTexture);
      }
  }
  if (mime->holdsType (CSMWorld::UniversalId::Type_Texture))
  {
      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          std::string textureFileName = uid.toString();
          createTexture(textureFileName);
          emit passBrushTexture(mBrushTexture);
      }
  }
}

void CSVRender::TerrainTextureMode::editTerrainTextureGrid(const WorldspaceHitResult& hit)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);
    if(allowLandTextureEditing(mCellId)==true) {}

    std::pair<CSMWorld::CellCoordinates, bool> cellCoordinates_pair = CSMWorld::CellCoordinates::fromId (mCellId);

    int cellX = cellCoordinates_pair.first.getX();
    int cellY = cellCoordinates_pair.first.getY();

    // The coordinates of hit in mCellId
    int xHitInCell (float(((hit.worldPos.x() - (cellX* cellSize)) * landTextureSize / cellSize) - 0.5));
    int yHitInCell (float(((hit.worldPos.y() - (cellY* cellSize)) * landTextureSize / cellSize) + 0.5));
    if (xHitInCell < 0)
    {
        xHitInCell = xHitInCell + landTextureSize;
        cellX = cellX - 1;
    }
    if (yHitInCell > 15)
    {
        yHitInCell = yHitInCell - landTextureSize;
        cellY = cellY + 1;
    }

    mCellId = "#" + std::to_string(cellX) + " " + std::to_string(cellY);
    if(allowLandTextureEditing(mCellId)==true) {}

    std::string iteratedCellId;

    int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

    std::size_t hashlocation = mBrushTexture.find("#");
    std::string mBrushTextureInt = mBrushTexture.substr (hashlocation+1);
    int brushInt = stoi(mBrushTexture.substr (hashlocation+1))+1; // All indices are offset by +1

    int rf = mBrushSize / 2;
    int r = mBrushSize / 2 + 1;
    int distance = 0;

    if (mBrushShape == 0)
    {
        CSMWorld::LandTexturesColumn::DataType mPointer = landTable.data(landTable.getModelIndex(mCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
        CSMWorld::LandTexturesColumn::DataType mNew(mPointer);

        if(allowLandTextureEditing(mCellId)==true)
        {
            mNew[yHitInCell*landTextureSize+xHitInCell] = brushInt;
            pushEditToCommand(mNew, document, landTable, mCellId);
        }
    }

    if (mBrushShape == 1)
    {
        int upperLeftCellX  = cellX - std::floor(r / landTextureSize);
        int upperLeftCellY  = cellY - std::floor(r / landTextureSize);
        if (xHitInCell - (r % landTextureSize) < 0) upperLeftCellX--;
        if (yHitInCell - (r % landTextureSize) < 0) upperLeftCellY--;

        int lowerrightCellX = cellX + std::floor(r / landTextureSize);
        int lowerrightCellY = cellY + std::floor(r / landTextureSize);
        if (xHitInCell + (r % landTextureSize) > landTextureSize - 1) lowerrightCellX++;
        if (yHitInCell + (r % landTextureSize) > landTextureSize - 1) lowerrightCellY++;

        for(int i_cell = upperLeftCellX; i_cell <= lowerrightCellX; i_cell++)
        {
            for(int j_cell = upperLeftCellY; j_cell <= lowerrightCellY; j_cell++)
            {
                iteratedCellId = "#" + std::to_string(i_cell) + " " + std::to_string(j_cell);
                if(allowLandTextureEditing(iteratedCellId)==true)
                {
                    CSMWorld::LandTexturesColumn::DataType mPointer = landTable.data(landTable.getModelIndex(iteratedCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                    CSMWorld::LandTexturesColumn::DataType mNew(mPointer);
                    for(int i = 0; i < landTextureSize; i++)
                    {
                        for(int j = 0; j < landTextureSize; j++)
                        {

                           if (i_cell == cellX && j_cell == cellY && abs(i-xHitInCell) < r && abs(j-yHitInCell) < r)
                            {
                                mNew[j*landTextureSize+i] = brushInt;
                            }
                            else
                            {
                                int distanceX(0);
                                int distanceY(0);
                                if (i_cell < cellX) distanceX = xHitInCell + landTextureSize * abs(i_cell-cellX) - i;
                                if (j_cell < cellY) distanceY = yHitInCell + landTextureSize * abs(j_cell-cellY) - j;
                                if (i_cell > cellX) distanceX = -xHitInCell + landTextureSize * abs(i_cell-cellX) + i;
                                if (j_cell > cellY) distanceY = -yHitInCell + landTextureSize * abs(j_cell-cellY) + j;
                                if (i_cell == cellX) distanceX = abs(i-xHitInCell);
                                if (j_cell == cellY) distanceY = abs(j-yHitInCell);
                                if (distanceX < r && distanceY < r) mNew[j*landTextureSize+i] = brushInt;
                            }
                        }
                    }
                    pushEditToCommand(mNew, document, landTable, iteratedCellId);
                }
            }
        }
    }

    if (mBrushShape == 2)
    {
        int upperLeftCellX  = cellX - std::floor(r / landTextureSize);
        int upperLeftCellY  = cellY - std::floor(r / landTextureSize);
        if (xHitInCell - (r % landTextureSize) < 0) upperLeftCellX--;
        if (yHitInCell - (r % landTextureSize) < 0) upperLeftCellY--;

        int lowerrightCellX = cellX + std::floor(r / landTextureSize);
        int lowerrightCellY = cellY + std::floor(r / landTextureSize);
        if (xHitInCell + (r % landTextureSize) > landTextureSize - 1) lowerrightCellX++;
        if (yHitInCell + (r % landTextureSize) > landTextureSize - 1) lowerrightCellY++;

        for(int i_cell = upperLeftCellX; i_cell <= lowerrightCellX; i_cell++)
        {
            for(int j_cell = upperLeftCellY; j_cell <= lowerrightCellY; j_cell++)
            {
                iteratedCellId = "#" + std::to_string(i_cell) + " " + std::to_string(j_cell);
                if(allowLandTextureEditing(iteratedCellId)==true)
                {
                    CSMWorld::LandTexturesColumn::DataType mPointer = landTable.data(landTable.getModelIndex(iteratedCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                    CSMWorld::LandTexturesColumn::DataType mNew(mPointer);
                    for(int i = 0; i < landTextureSize; i++)
                    {
                        for(int j = 0; j < landTextureSize; j++)
                        {

                            if (i_cell == cellX && j_cell == cellY && abs(i-xHitInCell) < r && abs(j-yHitInCell) < r)
                            {
                                int distanceX(0);
                                int distanceY(0);
                                if (i_cell < cellX) distanceX = xHitInCell + landTextureSize * abs(i_cell-cellX) - i;
                                if (j_cell < cellY) distanceY = yHitInCell + landTextureSize * abs(j_cell-cellY) - j;
                                if (i_cell > cellX) distanceX = -xHitInCell + landTextureSize* abs(i_cell-cellX) + i;
                                if (j_cell > cellY) distanceY = -yHitInCell + landTextureSize * abs(j_cell-cellY) + j;
                                if (i_cell == cellX) distanceX = abs(i-xHitInCell);
                                if (j_cell == cellY) distanceY = abs(j-yHitInCell);
                                distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                                if (distance < rf) mNew[j*landTextureSize+i] = brushInt;
                            }
                            else
                            {
                                int distanceX(0);
                                int distanceY(0);
                                if (i_cell < cellX) distanceX = xHitInCell + landTextureSize * abs(i_cell-cellX) - i;
                                if (j_cell < cellY) distanceY = yHitInCell + landTextureSize * abs(j_cell-cellY) - j;
                                if (i_cell > cellX) distanceX = -xHitInCell + landTextureSize * abs(i_cell-cellX) + i;
                                if (j_cell > cellY) distanceY = -yHitInCell + landTextureSize * abs(j_cell-cellY) + j;
                                if (i_cell == cellX) distanceX = abs(i-xHitInCell);
                                if (j_cell == cellY) distanceY = abs(j-yHitInCell);
                                distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                                if (distance < rf) mNew[j*landTextureSize+i] = brushInt;
                            }
                        }
                    }
                    pushEditToCommand(mNew, document, landTable, iteratedCellId);
                }
            }
        }
    }

    if (mBrushShape == 3)
    {
      // Not implemented
    }

}

void CSVRender::TerrainTextureMode::pushEditToCommand(CSMWorld::LandTexturesColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, std::string cellId)
{
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandTexturesIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, cellId));
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}

void CSVRender::TerrainTextureMode::createTexture(std::string textureFileName)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();

    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    QUndoStack& undoStack = document.getUndoStack();

    std::string newId;

    int counter=0;
    bool freeIndexFound = false;
    do {
        const size_t maxCounter = std::numeric_limits<uint16_t>::max() - 1;
        try
        {
            newId = CSMWorld::LandTexture::createUniqueRecordId(0, counter);
            if (ltexTable.getRecord(newId).isDeleted() == 0) counter = (counter + 1) % maxCounter;
        } catch (const std::exception& e)
        {
            newId = CSMWorld::LandTexture::createUniqueRecordId(0, counter);
            freeIndexFound = true;
        }
    } while (freeIndexFound == false);

    std::size_t idlocation = textureFileName.find("Texture: ");
    textureFileName = textureFileName.substr (idlocation + 9);

    QVariant textureNameVariant;

    QVariant textureFileNameVariant;
    textureFileNameVariant.setValue(QString::fromStdString(textureFileName));

    undoStack.beginMacro ("Add land texture record");

    undoStack.push (new CSMWorld::CreateCommand (ltexTable, newId));
    QModelIndex index(ltexTable.getModelIndex (newId, ltexTable.findColumnIndex (CSMWorld::Columns::ColumnId_Texture)));
    undoStack.push (new CSMWorld::ModifyCommand(ltexTable, index, textureFileNameVariant));
    undoStack.endMacro();
    mBrushTexture = newId;
}

bool CSVRender::TerrainTextureMode::allowLandTextureEditing(std::string cellId)
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

void CSVRender::TerrainTextureMode::dragMoveEvent (QDragMoveEvent *event) {
}

void CSVRender::TerrainTextureMode::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
}

void CSVRender::TerrainTextureMode::setBrushShape(int brushShape)
{
    mBrushShape = brushShape;
}

void CSVRender::TerrainTextureMode::setBrushTexture(std::string brushTexture)
{
    mBrushTexture = brushTexture;
}
