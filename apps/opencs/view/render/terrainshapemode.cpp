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
    mIsEditing(false),
    mTotalDiffY(0),
    mShapeEditTool(0),
    mShapeEditToolStrength(0)
{
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
        connect(mShapeBrushScenetool->mShapeBrushWindow->mToolSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setShapeEditTool(int)));
        connect(mShapeBrushScenetool->mShapeBrushWindow->mToolStrengthSlider, SIGNAL(valueChanged(int)), this, SLOT(setShapeEditToolStrength(int)));
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
    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    if (hit.hit && hit.tag == 0)
    {
    }
    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        paged->resetAllAlteredHeights();
        mTotalDiffY = 0;
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

    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    mDragMode = InteractionType_PrimaryEdit;

    if (hit.hit && hit.tag == 0)
    {
        mEditingPos = hit.worldPos;
        mIsEditing = true;
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
        mTotalDiffY += diffY;
        if (mIsEditing == true && mShapeEditTool == 0) editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(mEditingPos), true);
        if (mIsEditing == true && mShapeEditTool > 0) editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
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
        if (mIsEditing == true)
        {
            mTotalDiffY = 0;
            mIsEditing = false;
        }

        sort(mAlteredCells.begin(), mAlteredCells.end());
        mAlteredCells.erase(unique(mAlteredCells.begin(), mAlteredCells.end()), mAlteredCells.end());

        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));

        int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
        int landnormalsColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex);

        QUndoStack& undoStack = document.getUndoStack();

        undoStack.beginMacro ("Edit shape and normal records");

        for(CSMWorld::CellCoordinates cellCoordinates: mAlteredCells)
        {
            std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY());

            if (allowLandShapeEditing(cellId) == true)
            {
                limitHeightChanges(cellCoordinates);

                const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                CSMWorld::LandHeightsColumn::DataType landShapeNew(landShapePointer);
                const CSMWorld::LandNormalsColumn::DataType landNormalsPointer = landTable.data(landTable.getModelIndex(cellId, landnormalsColumn)).value<CSMWorld::LandNormalsColumn::DataType>();
                CSMWorld::LandNormalsColumn::DataType landNormalsNew(landNormalsPointer);

                //Normals code modified from tesanwynn
                float v1[3],
                    v2[3],
                    normal[3],
                    hyp;

                for(int i = 0; i < landSize; ++i)
                {
                    for(int j = 0; j < landSize; ++j)
                    {
                        if (CSVRender::PagedWorldspaceWidget *paged =
                            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
                        {
                            landShapeNew[j * landSize + i] = landShapePointer[j * landSize + i] + std::floor(paged->getCellAlteredHeights(cellCoordinates)[j * landSize + i]);
                        }

                        v1[0] = 128;
                        v1[1] = 0;
                        v1[2] = landShapeNew[j*landSize+i+1] - landShapeNew[j*landSize+i];

                        v2[0] = 0;
                        v2[1] = 128;
                        v2[2] = landShapeNew[(j+1)*landSize+i] - landShapeNew[j*landSize+i];

                        normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
                        normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
                        normal[2] = v1[0]*v2[1] - v1[1]*v2[0];

                        hyp = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]) / 127.0f;

                        normal[0] /= hyp; normal[1] /= hyp; normal[2] /= hyp;
                        landNormalsNew[(j*landSize+i)*3+0] = normal[0];
                        landNormalsNew[(j*landSize+i)*3+1] = normal[1];
                        landNormalsNew[(j*landSize+i)*3+2] = normal[2];
                    }
                }
                pushNormalsEditToCommand(landNormalsNew, document, landTable, cellId);
                pushEditToCommand(landShapeNew, document, landTable, cellId);
            }
        }
        undoStack.endMacro();
        mAlteredCells.clear();

        if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            paged->resetAllAlteredHeights();
            mTotalDiffY = 0;
        }
    }
}

void CSVRender::TerrainShapeMode::dragAborted()
{
    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        paged->resetAllAlteredHeights();
        mTotalDiffY = 0;
    }
}

void CSVRender::TerrainShapeMode::dragWheel (int diff, double speedFactor)
{
}

void CSVRender::TerrainShapeMode::editTerrainShapeGrid(std::pair<int, int> vertexCoords, bool dragOperation)
{
    int r = mBrushSize / 2;

    std::string cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
    CSMWorld::CellCoordinates cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;

    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        if (mShapeEditTool == 0) paged->resetAllAlteredHeights();
    }

    if(allowLandShapeEditing(cellId)==true)
    {
        if (mBrushShape == 0)
        {
            int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.first);
            int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.second);
            if (mShapeEditTool == 0) alterHeight(cellCoords, x, y, mTotalDiffY);
            if (mShapeEditTool == 1 || mShapeEditTool == 2) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
            if (mShapeEditTool == 3) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
        }

        if (mBrushShape == 1)
        {
            for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
            {
                for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
                {
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(i, j));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(i);
                    int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(j);
                    if (mShapeEditTool == 0) alterHeight(cellCoords, x, y, mTotalDiffY);
                    if (mShapeEditTool == 1 || mShapeEditTool == 2) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == 3) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
                }
            }
        }

        if (mBrushShape == 2)
        {
            for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
            {
                for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
                {
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(i, j));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int distanceX = abs(i - vertexCoords.first);
                    int distanceY = abs(j - vertexCoords.second);
                    int distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                    int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(i);
                    int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(j);
                    float distancePerRadius = 1.0f * distance / r;
                    float smoothedByDistance = 0.0f;
                    if (mShapeEditTool == 0) smoothedByDistance = mTotalDiffY - mTotalDiffY * (3 * distancePerRadius * distancePerRadius - 2 * distancePerRadius * distancePerRadius * distancePerRadius);
                    if (mShapeEditTool == 1 || mShapeEditTool == 2) smoothedByDistance = (r + mShapeEditToolStrength) - (r + mShapeEditToolStrength) * (3 * distancePerRadius * distancePerRadius - 2 * distancePerRadius * distancePerRadius * distancePerRadius);
                    if (distance < r)
                    {
                        if (mShapeEditTool >= 0 && mShapeEditTool < 3) alterHeight(cellCoords, x, y, smoothedByDistance);
                        if (mShapeEditTool == 3) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
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
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(vertexCoords.first + value.first, vertexCoords.second + value.second));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int x = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.first + value.first);
                    int y = CSMWorld::CellCoordinates::vertexSelectionToInCellCoords(vertexCoords.second + value.second);
                    if (mShapeEditTool == 0) alterHeight(cellCoords, x, y, mTotalDiffY);
                    if (mShapeEditTool == 1 || mShapeEditTool == 2) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == 3) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
                }
            }
        }

        sort(mAlteredCells.begin(), mAlteredCells.end());
        mAlteredCells.erase(unique(mAlteredCells.begin(), mAlteredCells.end()), mAlteredCells.end());
    }
}

void CSVRender::TerrainShapeMode::alterHeight(CSMWorld::CellCoordinates cellCoords, int inCellX, int inCellY, float alteredHeight, bool useTool)
{
    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    if(allowLandShapeEditing(cellId)==true)
    {
        mAlteredCells.emplace_back(cellCoords);
        if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            if (useTool)
            {
                if (mShapeEditTool == 0)
                {
                    // Get distance from modified land, alter land change based on zoom
                    osg::Vec3d eye, center, up;
                    paged->getCamera()->getViewMatrixAsLookAt(eye, center, up);
                    osg::Vec3d distance = eye - mEditingPos;
                    alteredHeight = alteredHeight * (distance.length() / 500);
                }
                if (mShapeEditTool == 1) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) + alteredHeight;
                if (mShapeEditTool == 2) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) - alteredHeight;
                if (mShapeEditTool == 3) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) + alteredHeight;
            }

            paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
            if (inCellX == 0 && inCellY == 0) // Bind the corner
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
                cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                if(allowLandShapeEditing(cellId)==true) // Open new cell
                {
                    inCellX = landSize - 1;
                    mAlteredCells.emplace_back(cellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                }

                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY() - 1); // move one cell back and up
                cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                if(allowLandShapeEditing(cellId)==true)  // Open new cell
                {
                    inCellX = 0;
                    inCellY = landSize - 1;
                    mAlteredCells.emplace_back(cellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                }

                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());  // move one cell left
                cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                if(allowLandShapeEditing(cellId)==true)  // Open new cell
                {
                    inCellX = landSize - 1;
                    inCellY = landSize - 1;
                    mAlteredCells.emplace_back(cellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                }
            }
            else if (inCellX == 0 && inCellY != 0) // Bind the last row to first row
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
                cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                if(allowLandShapeEditing(cellId)==true) {} // Open new cell
                inCellX = landSize - 1;
                mAlteredCells.emplace_back(cellCoords);
                paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
            }
            else if (inCellX != 0 && inCellY == 0) // Bind the last row to first row
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
                cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                if(allowLandShapeEditing(cellId)==true) {} // Open new cell
                inCellY = landSize - 1;
                mAlteredCells.emplace_back(cellCoords);
                paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
            }
        }
    }
}

void CSVRender::TerrainShapeMode::smoothHeight(CSMWorld::CellCoordinates cellCoords, int inCellX, int inCellY, int toolStrength)
{
    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
        int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);

        std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
        const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

        float thisAlteredHeight = 0.0f;
        if (paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) != nullptr)
        {
            thisAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY);
        }
        float thisHeight = landShapePointer[inCellY * landSize + inCellX];
        float leftHeight = 0.0f;
        float leftAlteredHeight = 0.0f;
        float upAlteredHeight = 0.0f;
        float rightHeight = 0.0f;
        float rightAlteredHeight = 0.0f;
        float downHeight = 0.0f;
        float downAlteredHeight = 0.0f;
        float upHeight = 0.0f;

        if(allowLandShapeEditing(cellId)==true)
        {
            if (inCellX == 0)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
                const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                leftHeight = landLeftShapePointer[inCellY * landSize + (landSize - 2)];
                if (paged->getCellAlteredHeight(cellCoords.move(-1, 0), inCellX, landSize - 2) != nullptr)
                {
                    leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(-1, 0), landSize - 2, inCellY);
                }
            }
            if (inCellY == 0)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
                const CSMWorld::LandHeightsColumn::DataType landUpShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                upHeight = landUpShapePointer[(landSize - 2) * landSize + inCellX];
                if (paged->getCellAlteredHeight(cellCoords.move(0, -1), inCellX, landSize - 2) != nullptr)
                {
                    upAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, -1), inCellX, landSize - 2);
                }
            }
            if (inCellX > 0)
            {
                leftHeight = landShapePointer[inCellY * landSize + inCellX - 1];
                leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX - 1, inCellY);
            }
            if (inCellY > 0)
            {
                upHeight = landShapePointer[(inCellY - 1) * landSize + inCellX];
                upAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY - 1);
            }

            if (inCellX == landSize - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
                const CSMWorld::LandHeightsColumn::DataType landRightShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                rightHeight = landRightShapePointer[inCellY * landSize + 1];
                if (paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY) != nullptr)
                {
                    rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY);
                }
            }
            if (inCellY == landSize - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);
                const CSMWorld::LandHeightsColumn::DataType landDownShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                downHeight = landDownShapePointer[1 * landSize + inCellX];
                if (paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1) != nullptr)
                {
                    downAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1);
                }
            }
            if (inCellX < landSize - 1)
            {
                rightHeight = landShapePointer[inCellY * landSize + inCellX + 1];
                rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX + 1, inCellY);
            }
            if (inCellY < landSize - 1)
            {
                downHeight = landShapePointer[(inCellY + 1) * landSize + inCellX];
                downAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY + 1);
            }

            float averageHeight = (upHeight + downHeight + rightHeight + leftHeight + upAlteredHeight + downAlteredHeight + rightAlteredHeight + leftAlteredHeight) / 4;
            if ((thisHeight + thisAlteredHeight) != averageHeight) mAlteredCells.emplace_back(cellCoords);
            if (toolStrength > abs(thisHeight + thisAlteredHeight - averageHeight) && toolStrength > 8.0f) toolStrength = abs(thisHeight + thisAlteredHeight - averageHeight);
            if (thisHeight + thisAlteredHeight > averageHeight) alterHeight(cellCoords, inCellX, inCellY, -toolStrength);
            if (thisHeight + thisAlteredHeight < averageHeight) alterHeight(cellCoords, inCellX, inCellY, +toolStrength);
        }
    }
}

void CSVRender::TerrainShapeMode::limitHeightChanges(CSMWorld::CellCoordinates cellCoords)
{
    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();
        CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
        int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);

        std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
        const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

        int limitHeightChange = 1000.0f; // Limited by save format

        // Go through vertices, and limit consecutive height changes
        for(int inCellX = 0; inCellX < landSize; ++inCellX)
        {
            for(int inCellY = 0; inCellY < landSize; ++inCellY)
            {
                float alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY);
                float thisHeight = landShapePointer[inCellY * landSize + inCellX] + alteredHeight;
                float leftHeight = 0.0f;
                float leftAlteredHeight = 0.0f;
                float leftBindingHeight = 0.0f;
                float leftBindingAlteredHeight = 0.0f;
                float upHeight = 0.0f;
                float upAlteredHeight = 0.0f;
                float upBindingHeight = 0.0f;
                float upBindingAlteredHeight = 0.0f;

                // Get height points
                if (inCellX == 0)
                {
                    cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
                    const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    leftHeight = landLeftShapePointer[inCellY * landSize + (landSize - 2)];
                    leftBindingHeight = landLeftShapePointer[inCellY * landSize + (landSize - 1)];
                    if (paged->getCellAlteredHeight(cellCoords.move(-1,0), landSize - 2, inCellY) != nullptr)
                    {
                        leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(-1,0), landSize - 2, inCellY);
                        leftHeight = landLeftShapePointer[inCellY * landSize + (landSize - 2)] + leftAlteredHeight;
                        leftBindingAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(-1,0), landSize - 1, inCellY);
                        leftBindingHeight = landLeftShapePointer[inCellY * landSize + (landSize - 1)] + leftBindingAlteredHeight;
                    }
                }
                if (inCellY == 0)
                {
                    cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
                    const CSMWorld::LandHeightsColumn::DataType landUpShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    upHeight = landUpShapePointer[(landSize - 2) * landSize + inCellX];
                    upBindingHeight = landUpShapePointer[(landSize - 1) * landSize + inCellX];
                    if (paged->getCellAlteredHeight(cellCoords.move(0,-1), inCellX, landSize - 2) != nullptr)
                    {
                        upAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0,-1), inCellX, landSize - 2);
                        upHeight = landUpShapePointer[(landSize - 2) * landSize + inCellX] + upAlteredHeight;
                        upBindingAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0,-1), inCellX, landSize - 1);
                        upBindingHeight = landUpShapePointer[(landSize - 1) * landSize + inCellX] + upBindingAlteredHeight;
                    }
                }
                if (inCellX != 0)
                {
                    leftHeight = landShapePointer[inCellY * landSize + inCellX - 1] + *paged->getCellAlteredHeight(cellCoords, inCellX - 1, inCellY);
                }
                if (inCellY != 0)
                {
                    upHeight = landShapePointer[(inCellY - 1) * landSize + inCellX] + *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY - 1);
                }

                // Check for height limits
                bool doChange = false;
                if (thisHeight - upHeight > limitHeightChange)
                {
                    alteredHeight = upHeight + limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                    doChange = true;
                }
                if (thisHeight - upHeight < -limitHeightChange)
                {
                    alteredHeight = upHeight - limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                    doChange = true;
                }
                if (thisHeight - leftHeight > limitHeightChange)
                {
                    alteredHeight = leftHeight + limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                    doChange = true;
                }
                if (thisHeight - leftHeight < -limitHeightChange)
                {
                    alteredHeight = leftHeight - limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                    doChange = true;
                }

                // Bind first row to last row, regardless of limit breaks between cells
                if (inCellY == 0 && thisHeight != upBindingHeight)
                {
                    alteredHeight = upBindingAlteredHeight;
                    doChange = true;
                }
                if (inCellX == 0 && thisHeight != leftBindingHeight)
                {
                    alteredHeight = leftBindingAlteredHeight;
                    doChange = true;

                    //First column has to be limited no matter what to preserve data corruption
                    if (thisHeight - upHeight > limitHeightChange)
                        alteredHeight = upHeight + limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                    if (thisHeight - upHeight < -limitHeightChange)
                        alteredHeight = upHeight - limitHeightChange - landShapePointer[inCellY * landSize + inCellX];
                }

                if (doChange == true) alterHeight(cellCoords, inCellX, inCellY, alteredHeight, false);
            }
        }
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

void CSVRender::TerrainShapeMode::pushEditToCommand(CSMWorld::LandHeightsColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, std::string cellId)
{
    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandHeightsIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}

void CSVRender::TerrainShapeMode::pushNormalsEditToCommand(CSMWorld::LandNormalsColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, std::string cellId)
{
    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandNormalsIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}

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

void CSVRender::TerrainShapeMode::setShapeEditTool(int shapeEditTool)
{
    mShapeEditTool = shapeEditTool;
}

void CSVRender::TerrainShapeMode::setShapeEditToolStrength(int shapeEditToolStrength)
{
    mShapeEditToolStrength = shapeEditToolStrength;
}

CSVRender::PagedWorldspaceWidget& CSVRender::TerrainShapeMode::getPagedWorldspaceWidget()
{
    return dynamic_cast<PagedWorldspaceWidget&>(getWorldspaceWidget());
}
