#include "terrainshapemode.hpp"

#include <algorithm>
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
#include <osg/Vec3f>

#include <components/esm/loadland.hpp>
#include <components/debug/debuglog.hpp>

#include "../widget/brushshapes.hpp"
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
    mBrushSize(1),
    mBrushShape(CSVWidget::BrushShape_Point),
    mShapeBrushScenetool(0),
    mDragMode(InteractionType_None),
    mParentNode(parentNode),
    mIsEditing(false),
    mTotalDiffY(0),
    mShapeEditTool(ShapeEditTool_Drag),
    mShapeEditToolStrength(8),
    mTargetHeight(0)
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
        connect(mShapeBrushScenetool->mShapeBrushWindow, SIGNAL(passBrushShape(CSVWidget::BrushShape)), this, SLOT(setBrushShape(CSVWidget::BrushShape)));
        connect(mShapeBrushScenetool->mShapeBrushWindow->mSizeSliders->mBrushSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrushSize(int)));
        connect(mShapeBrushScenetool->mShapeBrushWindow->mToolSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setShapeEditTool(int)));
        connect(mShapeBrushScenetool->mShapeBrushWindow->mToolStrengthSlider, SIGNAL(valueChanged(int)), this, SLOT(setShapeEditToolStrength(int)));
    }

    EditMode::activate(toolbar);
    toolbar->addTool (mShapeBrushScenetool);
}

void CSVRender::TerrainShapeMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainShapeMode::primaryOpenPressed (const WorldspaceHitResult& hit) // Apply changes here
{
}

void CSVRender::TerrainShapeMode::primaryEditPressed(const WorldspaceHitResult& hit)
{
    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);

    if (hit.hit && hit.tag == 0)
    {
        if (mShapeEditTool == ShapeEditTool_Flatten)
            setFlattenToolTargetHeight(hit);
        if (mDragMode == InteractionType_PrimaryEdit && mShapeEditTool != ShapeEditTool_Drag)
        {
            std::string cellId = getWorldspaceWidget().getCellId (hit.worldPos);
            editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
            applyTerrainEditChanges();
        }

        if (mDragMode == InteractionType_PrimarySelect)
        {
            selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 0, true);
        }

        if (mDragMode == InteractionType_SecondarySelect)
        {
            selectTerrainShapes(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), 1, true);
        }
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
        if (mShapeEditTool == ShapeEditTool_Flatten)
            setFlattenToolTargetHeight(hit);
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
        if (mIsEditing == true && mShapeEditTool == ShapeEditTool_Drag) editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(mEditingPos), true);
        if (mIsEditing == true && mShapeEditTool != ShapeEditTool_Drag) editTerrainShapeGrid(CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos), true);
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

        applyTerrainEditChanges();

        if (CSVRender::PagedWorldspaceWidget *paged = dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
            paged->resetAllAlteredHeights();
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


void CSVRender::TerrainShapeMode::applyTerrainEditChanges()
{
    std::sort(mAlteredCells.begin(), mAlteredCells.end());
    mAlteredCells.erase(std::unique(mAlteredCells.begin(), mAlteredCells.end()), mAlteredCells.end());

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    int landMapLodColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandMapLodIndex);
    int landnormalsColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex);

    QUndoStack& undoStack = document.getUndoStack();

    bool passing = false;
    int passes = 0;

    while (passing == false) // Multiple passes are needed when steepness problems arise for both x and y axis simultaneously
    {
        passing = true;
        for(CSMWorld::CellCoordinates cellCoordinates: mAlteredCells)
        {
            limitAlteredHeights(cellCoordinates);
        }
        std::reverse(mAlteredCells.begin(), mAlteredCells.end()); //Instead of alphabetical order, this should be fixed to sort cells by cell coordinates
        for(CSMWorld::CellCoordinates cellCoordinates: mAlteredCells)
        {
            if (!limitAlteredHeights(cellCoordinates, true)) passing = false;
        }
        ++passes;
        if (passes > 2)
        {
            Log(Debug::Warning) << "Warning: User edit exceeds accepted slope steepness. Automatic limiting has failed, edit has been discarded.";
            if (CSVRender::PagedWorldspaceWidget *paged =
                dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
            {
                paged->resetAllAlteredHeights();
                mAlteredCells.clear();
                return;
            }
        }
    }

    undoStack.beginMacro ("Edit shape and normal records");

    for(CSMWorld::CellCoordinates cellCoordinates: mAlteredCells)
    {
        std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY());
        undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, cellId));
        const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        const CSMWorld::LandMapLodColumn::DataType landMapLodPointer = landTable.data(landTable.getModelIndex(cellId, landMapLodColumn)).value<CSMWorld::LandMapLodColumn::DataType>();
        CSMWorld::LandHeightsColumn::DataType landShapeNew(landShapePointer);
        CSMWorld::LandMapLodColumn::DataType mapLodShapeNew(landMapLodPointer);
        for(int i = 0; i < ESM::Land::LAND_SIZE; ++i)
        {
            for(int j = 0; j < ESM::Land::LAND_SIZE; ++j)
            {
                if (CSVRender::PagedWorldspaceWidget *paged =
                    dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
                {
                    if (paged->getCellAlteredHeight(cellCoordinates, i, j))
                        landShapeNew[j * ESM::Land::LAND_SIZE + i] = landShapePointer[j * ESM::Land::LAND_SIZE + i] + *paged->getCellAlteredHeight(cellCoordinates, i, j);
                    else
                        landShapeNew[j * ESM::Land::LAND_SIZE + i] = 0;
                }
            }
        }
        for(int i = 0; i < ESM::Land::LAND_GLOBAL_MAP_LOD_SIZE; ++i)
        {
            mapLodShapeNew[i] = landMapLodPointer[i]; //TO-DO: generate a new mWnam based on new heights
        }
        if (allowLandShapeEditing(cellId) == true)
        {
            pushEditToCommand(landShapeNew, document, landTable, cellId);
            pushLodToCommand(mapLodShapeNew, document, landTable, cellId);
        }
    }

    for(CSMWorld::CellCoordinates cellCoordinates: mAlteredCells)
    {
        std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY());
        const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        const CSMWorld::LandHeightsColumn::DataType landRightShapePointer = landTable.data(landTable.getModelIndex(CSMWorld::CellCoordinates::generateId(cellCoordinates.getX() + 1, cellCoordinates.getY()), landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        const CSMWorld::LandHeightsColumn::DataType landDownShapePointer = landTable.data(landTable.getModelIndex(CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY() + 1), landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
        const CSMWorld::LandNormalsColumn::DataType landNormalsPointer = landTable.data(landTable.getModelIndex(cellId, landnormalsColumn)).value<CSMWorld::LandNormalsColumn::DataType>();
        CSMWorld::LandNormalsColumn::DataType landNormalsNew(landNormalsPointer);

        for(int i = 0; i < ESM::Land::LAND_SIZE; ++i)
        {
            for(int j = 0; j < ESM::Land::LAND_SIZE; ++j)
            {
                osg::Vec3f v1;
                osg::Vec3f v2;
                osg::Vec3f normal;
                float hyp;

                v1.x() = 128;
                v1.y() = 0;
                if (i < ESM::Land::LAND_SIZE - 1) v1.z() = landShapePointer[j * ESM::Land::LAND_SIZE + i + 1] - landShapePointer[j * ESM::Land::LAND_SIZE + i];
                else
                {
                    bool noCell = document.getData().getCells().searchId (CSMWorld::CellCoordinates::generateId(cellCoordinates.getX() + 1, cellCoordinates.getY())) == -1;
                    bool noLand = document.getData().getLand().searchId (CSMWorld::CellCoordinates::generateId(cellCoordinates.getX() + 1, cellCoordinates.getY())) == -1;
                    if (!noLand && !noCell)
                        v1.z() = landRightShapePointer[j * ESM::Land::LAND_SIZE + 1] - landShapePointer[j * ESM::Land::LAND_SIZE + i];
                    else
                        v1.z() = 0;
                }

                v2.x() = 0;
                v2.y() = 128;
                if (j < ESM::Land::LAND_SIZE - 1) v2.z() = landShapePointer[(j + 1) * ESM::Land::LAND_SIZE + i] - landShapePointer[j * ESM::Land::LAND_SIZE + i];
                else
                {
                    bool noCell = document.getData().getCells().searchId (CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY() + 1)) == -1;
                    bool noLand = document.getData().getLand().searchId (CSMWorld::CellCoordinates::generateId(cellCoordinates.getX(), cellCoordinates.getY() + 1)) == -1;
                    if (!noLand && !noCell)
                        v2.z() = landDownShapePointer[ESM::Land::LAND_SIZE + i] - landShapePointer[j * ESM::Land::LAND_SIZE + i];
                    else
                        v2.z() = 0;
                }

                normal = v1 ^ v2;

                hyp = normal.length() / 127.0f;

                normal /= hyp;

                landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 0] = normal.x();
                landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 1] = normal.y();
                landNormalsNew[(j * ESM::Land::LAND_SIZE + i) * 3 + 2] = normal.z();
            }
        }
        if (allowLandShapeEditing(cellId) == true) pushNormalsEditToCommand(landNormalsNew, document, landTable, cellId);
    }
    undoStack.endMacro();
    mAlteredCells.clear();
}

void CSVRender::TerrainShapeMode::editTerrainShapeGrid(const std::pair<int, int>& vertexCoords, bool dragOperation)
{
    int r = mBrushSize / 2;
    if (r == 0) r = 1; // Prevent division by zero later, which might happen when mBrushSize == 1

    std::string cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
    CSMWorld::CellCoordinates cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;

    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        if (mShapeEditTool == ShapeEditTool_Drag) paged->resetAllAlteredHeights();
    }

    if(allowLandShapeEditing(cellId)==true)
    {
        if (mBrushShape == CSVWidget::BrushShape_Point)
        {
            int x = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.first);
            int y = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.second);
            if (mShapeEditTool == ShapeEditTool_Drag) alterHeight(cellCoords, x, y, mTotalDiffY);
            if (mShapeEditTool == ShapeEditTool_PaintToRaise || mShapeEditTool == ShapeEditTool_PaintToLower) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
            if (mShapeEditTool == ShapeEditTool_Smooth) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
            if (mShapeEditTool == ShapeEditTool_Flatten) flattenHeight(cellCoords, x, y, mShapeEditToolStrength, mTargetHeight);
        }

        if (mBrushShape == CSVWidget::BrushShape_Square)
        {
            for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
            {
                for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
                {
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(i, j));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int x = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(i);
                    int y = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(j);
                    if (mShapeEditTool == ShapeEditTool_Drag) alterHeight(cellCoords, x, y, mTotalDiffY);
                    if (mShapeEditTool == ShapeEditTool_PaintToRaise || mShapeEditTool == ShapeEditTool_PaintToLower) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == ShapeEditTool_Smooth) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == ShapeEditTool_Flatten) flattenHeight(cellCoords, x, y, mShapeEditToolStrength, mTargetHeight);
                }
            }
        }

        if (mBrushShape == CSVWidget::BrushShape_Circle)
        {
            for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
            {
                for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
                {
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(i, j));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int distanceX = abs(i - vertexCoords.first);
                    int distanceY = abs(j - vertexCoords.second);
                    float distance = sqrt(pow(distanceX, 2)+pow(distanceY, 2));
                    int x = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(i);
                    int y = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(j);
                    float distancePerRadius = 1.0f * distance / r;
                    float smoothedByDistance = 0.0f;
                    if (mShapeEditTool == ShapeEditTool_Drag) smoothedByDistance = mTotalDiffY - mTotalDiffY * (3 * distancePerRadius * distancePerRadius - 2 * distancePerRadius * distancePerRadius * distancePerRadius);
                    if (mShapeEditTool == ShapeEditTool_PaintToRaise || mShapeEditTool == ShapeEditTool_PaintToLower) smoothedByDistance = (r + mShapeEditToolStrength) - (r + mShapeEditToolStrength) * (3 * distancePerRadius * distancePerRadius - 2 * distancePerRadius * distancePerRadius * distancePerRadius);
                    if (distance <= r)
                    {
                        if (mShapeEditTool == ShapeEditTool_Drag || mShapeEditTool == ShapeEditTool_PaintToRaise || mShapeEditTool == ShapeEditTool_PaintToLower)
                            alterHeight(cellCoords, x, y, smoothedByDistance);
                        if (mShapeEditTool == ShapeEditTool_Smooth) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
                        if (mShapeEditTool == ShapeEditTool_Flatten) flattenHeight(cellCoords, x, y, mShapeEditToolStrength, mTargetHeight);
                    }
                }
            }
        }
        if (mBrushShape == CSVWidget::BrushShape_Custom)
        {
            if(!mCustomBrushShape.empty())
            {
                for(auto const& value: mCustomBrushShape)
                {
                    cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(std::make_pair(vertexCoords.first + value.first, vertexCoords.second + value.second));
                    cellCoords = CSMWorld::CellCoordinates::fromId(cellId).first;
                    int x = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.first + value.first);
                    int y = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.second + value.second);
                    if (mShapeEditTool == ShapeEditTool_Drag) alterHeight(cellCoords, x, y, mTotalDiffY);
                    if (mShapeEditTool == ShapeEditTool_PaintToRaise || mShapeEditTool == ShapeEditTool_PaintToLower) alterHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == ShapeEditTool_Smooth) smoothHeight(cellCoords, x, y, mShapeEditToolStrength);
                    if (mShapeEditTool == ShapeEditTool_Flatten) flattenHeight(cellCoords, x, y, mShapeEditToolStrength, mTargetHeight);
                }
            }
        }

    }
}

void CSVRender::TerrainShapeMode::setFlattenToolTargetHeight(const WorldspaceHitResult& hit)
{
    std::pair<int, int> vertexCoords = CSMWorld::CellCoordinates::toVertexCoords(hit.worldPos);
    std::string cellId = CSMWorld::CellCoordinates::vertexGlobalToCellId(vertexCoords);
    int inCellX = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.first);
    int inCellY = CSMWorld::CellCoordinates::vertexGlobalToInCellCoords(vertexCoords.second);

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const CSMWorld::LandHeightsColumn::DataType landShapePointer =
        landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

    mTargetHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX];
}


void CSVRender::TerrainShapeMode::alterHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float alteredHeight, bool useTool)
{
    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    std::string cellLeftId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
    std::string cellRightId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
    std::string cellUpId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
    std::string cellDownId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);
    std::string cellUpLeftId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY() - 1);
    std::string cellUpRightId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY() - 1);
    std::string cellDownLeftId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY() + 1);
    std::string cellDownRightId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY() + 1);

    if(allowLandShapeEditing(cellId)==true)
    {
        if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            if (useTool)
            {
                mAlteredCells.emplace_back(cellCoords);
                if (mShapeEditTool == ShapeEditTool_Drag)
                {
                    // Get distance from modified land, alter land change based on zoom
                    osg::Vec3d eye, center, up;
                    paged->getCamera()->getViewMatrixAsLookAt(eye, center, up);
                    osg::Vec3d distance = eye - mEditingPos;
                    alteredHeight = alteredHeight * (distance.length() / 500);
                }
                if (mShapeEditTool == ShapeEditTool_PaintToRaise) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) + alteredHeight;
                if (mShapeEditTool == ShapeEditTool_PaintToLower) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) - alteredHeight;
                if (mShapeEditTool == ShapeEditTool_Smooth) alteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) + alteredHeight;
            }

            if (inCellX != 0 && inCellY != 0 && inCellX != ESM::Land::LAND_SIZE - 1 && inCellY != ESM::Land::LAND_SIZE - 1)
                paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);

            // Change values of cornering cells
            if (inCellX == 0 && inCellY == 0)
            {
                if(allowLandShapeEditing(cellUpLeftId) && allowLandShapeEditing(cellLeftId) && allowLandShapeEditing(cellUpId))
                {
                    CSMWorld::CellCoordinates cornerCellCoords = cellCoords.move(-1, -1);
                    if (useTool) mAlteredCells.emplace_back(cornerCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), cornerCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(cornerCellCoords);
                    paged->setCellAlteredHeight(cornerCellCoords, ESM::Land::LAND_SIZE - 1, ESM::Land::LAND_SIZE - 1, alteredHeight);
                } else return;
            }
            else if (inCellX == 0 && inCellY == ESM::Land::LAND_SIZE - 1)
            {
                if(allowLandShapeEditing(cellDownLeftId) && allowLandShapeEditing(cellLeftId) && allowLandShapeEditing(cellDownId))
                {
                    CSMWorld::CellCoordinates cornerCellCoords = cellCoords.move(-1, 1);
                    if (useTool) mAlteredCells.emplace_back(cornerCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), cornerCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(cornerCellCoords);
                    paged->setCellAlteredHeight(cornerCellCoords, ESM::Land::LAND_SIZE - 1, 0, alteredHeight);
                } else return;
            }
            else if (inCellX == ESM::Land::LAND_SIZE - 1 && inCellY == 0)
            {
                if(allowLandShapeEditing(cellUpRightId) && allowLandShapeEditing(cellRightId) && allowLandShapeEditing(cellUpId))
                {
                    CSMWorld::CellCoordinates cornerCellCoords = cellCoords.move(1, -1);
                    if (useTool) mAlteredCells.emplace_back(cornerCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), cornerCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(cornerCellCoords);
                    paged->setCellAlteredHeight(cornerCellCoords, 0, ESM::Land::LAND_SIZE - 1, alteredHeight);
                } else return;
            }
            else if (inCellX == ESM::Land::LAND_SIZE - 1 && inCellY == ESM::Land::LAND_SIZE - 1)
            {
                if(allowLandShapeEditing(cellDownRightId) && allowLandShapeEditing(cellRightId) && allowLandShapeEditing(cellDownId))
                {
                    CSMWorld::CellCoordinates cornerCellCoords = cellCoords.move(1, 1);
                    if (useTool) mAlteredCells.emplace_back(cornerCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), cornerCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(cornerCellCoords);
                    paged->setCellAlteredHeight(cornerCellCoords, 0, 0, alteredHeight);
                } else return;
            }

            // Change values of edging cells
            if (inCellX == 0)
            {
                if(allowLandShapeEditing(cellLeftId)==true)
                {
                    CSMWorld::CellCoordinates edgeCellCoords = cellCoords.move(-1, 0);
                    if (useTool) mAlteredCells.emplace_back(edgeCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), edgeCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(edgeCellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                    paged->setCellAlteredHeight(edgeCellCoords, ESM::Land::LAND_SIZE - 1, inCellY, alteredHeight);
                }
            }
            if (inCellY == 0)
            {
                if(allowLandShapeEditing(cellUpId)==true)
                {
                    CSMWorld::CellCoordinates edgeCellCoords = cellCoords.move(0, -1);
                    if (useTool) mAlteredCells.emplace_back(edgeCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), edgeCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(edgeCellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                    paged->setCellAlteredHeight(edgeCellCoords, inCellX, ESM::Land::LAND_SIZE - 1, alteredHeight);
                }
            }

            if (inCellX == ESM::Land::LAND_SIZE - 1)
            {
                if(allowLandShapeEditing(cellRightId)==true)
                {
                    CSMWorld::CellCoordinates edgeCellCoords = cellCoords.move(1, 0);
                    if (useTool) mAlteredCells.emplace_back(edgeCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), edgeCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(edgeCellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                    paged->setCellAlteredHeight(edgeCellCoords, 0, inCellY, alteredHeight);
                }
            }
            if (inCellY == ESM::Land::LAND_SIZE - 1)
            {
                if(allowLandShapeEditing(cellDownId)==true)
                {
                    CSMWorld::CellCoordinates edgeCellCoords = cellCoords.move(0, 1);
                    if (useTool) mAlteredCells.emplace_back(edgeCellCoords);
                    else if (!(std::find(mAlteredCells.begin(), mAlteredCells.end(), edgeCellCoords) != mAlteredCells.end()))
                        mAlteredCells.emplace_back(edgeCellCoords);
                    paged->setCellAlteredHeight(cellCoords, inCellX, inCellY, alteredHeight);
                    paged->setCellAlteredHeight(edgeCellCoords, inCellX, 0, alteredHeight);
                }
            }

        }
    }
}

void CSVRender::TerrainShapeMode::smoothHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, int toolStrength)
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

        // ### Variable naming key ###
        // Variables here hold either the real value, or the altered value of current edit.
        // this = this Cell
        // left = x - 1, up = y - 1, right = x + 1, down = y + 1
        // Altered = transient edit (in current edited)
        float thisAlteredHeight = 0.0f;
        if (paged->getCellAlteredHeight(cellCoords, inCellX, inCellY) != nullptr)
            thisAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY);
        float thisHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX];
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
            //Get key values for calculating average, handle cell edges, check for null pointers
            if (inCellX == 0)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
                const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                leftHeight = landLeftShapePointer[inCellY * ESM::Land::LAND_SIZE + (ESM::Land::LAND_SIZE - 2)];
                if (paged->getCellAlteredHeight(cellCoords.move(-1, 0), inCellX, ESM::Land::LAND_SIZE - 2))
                    leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(-1, 0), ESM::Land::LAND_SIZE - 2, inCellY);
            }
            if (inCellY == 0)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
                const CSMWorld::LandHeightsColumn::DataType landUpShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                upHeight = landUpShapePointer[(ESM::Land::LAND_SIZE - 2) * ESM::Land::LAND_SIZE + inCellX];
                if (paged->getCellAlteredHeight(cellCoords.move(0, -1), inCellX, ESM::Land::LAND_SIZE - 2))
                    upAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, -1), inCellX, ESM::Land::LAND_SIZE - 2);
            }
            if (inCellX > 0)
            {
                leftHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX - 1];
                leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX - 1, inCellY);
            }
            if (inCellY > 0)
            {
                upHeight = landShapePointer[(inCellY - 1) * ESM::Land::LAND_SIZE + inCellX];
                upAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY - 1);
            }
            if (inCellX == ESM::Land::LAND_SIZE - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
                const CSMWorld::LandHeightsColumn::DataType landRightShapePointer =
                    landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                rightHeight = landRightShapePointer[inCellY * ESM::Land::LAND_SIZE + 1];
                if (paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY))
                {
                    rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY);
                }
            }
            if (inCellY == ESM::Land::LAND_SIZE - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);
                const CSMWorld::LandHeightsColumn::DataType landDownShapePointer =
                    landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                downHeight = landDownShapePointer[1 * ESM::Land::LAND_SIZE + inCellX];
                if (paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1))
                {
                    downAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1);
                }
            }
            if (inCellX < ESM::Land::LAND_SIZE - 1)
            {
                rightHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX + 1];
                if(paged->getCellAlteredHeight(cellCoords, inCellX + 1, inCellY))
                    rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX + 1, inCellY);
            }
            if (inCellY < ESM::Land::LAND_SIZE - 1)
            {
                downHeight = landShapePointer[(inCellY + 1) * ESM::Land::LAND_SIZE + inCellX];
                if(paged->getCellAlteredHeight(cellCoords, inCellX, inCellY + 1))
                    downAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY + 1);
            }

            float averageHeight = (upHeight + downHeight + rightHeight + leftHeight +
                upAlteredHeight + downAlteredHeight + rightAlteredHeight + leftAlteredHeight) / 4;
            if ((thisHeight + thisAlteredHeight) != averageHeight) mAlteredCells.emplace_back(cellCoords);
            if (toolStrength > abs(thisHeight + thisAlteredHeight - averageHeight)) toolStrength = abs(thisHeight + thisAlteredHeight - averageHeight);
            if (thisHeight + thisAlteredHeight > averageHeight) alterHeight(cellCoords, inCellX, inCellY, - toolStrength);
            if (thisHeight + thisAlteredHeight < averageHeight) alterHeight(cellCoords, inCellX, inCellY, + toolStrength);
        }
    }
}

void CSVRender::TerrainShapeMode::flattenHeight(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, int toolStrength, int targetHeight)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);

    float thisHeight = 0.0f;
    float thisAlteredHeight = 0.0f;

    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    bool noCell = document.getData().getCells().searchId (cellId) == -1;
    bool noLand = document.getData().getLand().searchId (cellId) == -1;

    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        if (!noCell && !noLand)
        {
            const CSMWorld::LandHeightsColumn::DataType landShapePointer =
                landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

            if(paged->getCellAlteredHeight(cellCoords, inCellX, inCellY))
                thisAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY);
            thisHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX];
        }
    }

    if (toolStrength > abs(thisHeight - targetHeight) && toolStrength > 8.0f) toolStrength =
        abs(thisHeight - targetHeight); //Cut down excessive changes
    if (thisHeight + thisAlteredHeight > targetHeight) alterHeight(cellCoords, inCellX, inCellY, thisAlteredHeight - toolStrength);
    if (thisHeight + thisAlteredHeight < targetHeight) alterHeight(cellCoords, inCellX, inCellY, thisAlteredHeight + toolStrength);
}

void CSVRender::TerrainShapeMode::updateKeyHeightValues(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float* thisHeight,
    float* thisAlteredHeight, float* leftHeight, float* leftAlteredHeight, float* upHeight, float* upAlteredHeight, float* rightHeight,
    float* rightAlteredHeight, float* downHeight, float* downAlteredHeight)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);

    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    std::string cellLeftId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
    std::string cellUpId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
    std::string cellRightId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
    std::string cellDownId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);
    bool noCell = document.getData().getCells().searchId (cellId) == -1;
    bool noLand = document.getData().getLand().searchId (cellId) == -1;
    bool noLeftCell = document.getData().getCells().searchId (cellLeftId) == -1;
    bool noLeftLand = document.getData().getLand().searchId (cellLeftId) == -1;
    bool noUpCell = document.getData().getCells().searchId (cellUpId) == -1;
    bool noUpLand = document.getData().getLand().searchId (cellUpId) == -1;
    bool noRightCell = document.getData().getCells().searchId (cellRightId) == -1;
    bool noRightLand = document.getData().getLand().searchId (cellRightId) == -1;
    bool noDownCell = document.getData().getCells().searchId (cellDownId) == -1;
    bool noDownLand = document.getData().getLand().searchId (cellDownId) == -1;

    *thisHeight = 0.0f; // real + altered height
    *thisAlteredHeight = 0.0f;  // only altered height
    *leftHeight = 0.0f;
    *leftAlteredHeight = 0.0f;
    *upHeight = 0.0f;
    *upAlteredHeight = 0.0f;
    *rightHeight = 0.0f;
    *rightAlteredHeight = 0.0f;
    *downHeight = 0.0f;
    *downAlteredHeight = 0.0f;

    if (CSVRender::PagedWorldspaceWidget *paged =
        dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
    {
        if (!noCell && !noLand)
        {
            const CSMWorld::LandHeightsColumn::DataType landShapePointer =
                landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

            if(paged->getCellAlteredHeight(cellCoords, inCellX, inCellY))
                *thisAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY);
            *thisHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX] + *thisAlteredHeight;

            // Default to the same value as thisHeight, which happens in the case of cell edge where next cell/land is not found,
            // which is to prevent unnecessary action at limitHeightChange().
            *leftHeight = *thisHeight;
            *upHeight = *thisHeight;
            *rightHeight = *thisHeight;
            *downHeight = *thisHeight;

            //If at edge, get values from neighboring cell
            if (inCellX == 0)
            {
                if(!noLeftCell && !noLeftLand)
                {
                    const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer =
                        landTable.data(landTable.getModelIndex(cellLeftId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    *leftHeight = landLeftShapePointer[inCellY * ESM::Land::LAND_SIZE + (ESM::Land::LAND_SIZE - 2)];
                    if (paged->getCellAlteredHeight(cellCoords.move(-1, 0), ESM::Land::LAND_SIZE - 2, inCellY))
                    {
                        *leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(-1, 0), ESM::Land::LAND_SIZE - 2, inCellY);
                        *leftHeight += *leftAlteredHeight;
                    }
                }
            }
            if (inCellY == 0)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
                if(!noUpCell && !noUpLand)
                {
                    const CSMWorld::LandHeightsColumn::DataType landUpShapePointer =
                        landTable.data(landTable.getModelIndex(cellUpId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    *upHeight = landUpShapePointer[(ESM::Land::LAND_SIZE - 2) * ESM::Land::LAND_SIZE + inCellX];
                    if (paged->getCellAlteredHeight(cellCoords.move(0,-1), inCellX, ESM::Land::LAND_SIZE - 2))
                    {
                        *upAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, -1), inCellX, ESM::Land::LAND_SIZE - 2);
                        *upHeight += *upAlteredHeight;
                    }
                }
            }
            if (inCellX == ESM::Land::LAND_SIZE - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
                if(!noRightCell && !noRightLand)
                {
                    const CSMWorld::LandHeightsColumn::DataType landRightShapePointer =
                        landTable.data(landTable.getModelIndex(cellRightId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    *rightHeight = landRightShapePointer[inCellY * ESM::Land::LAND_SIZE + 1];
                    if (paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY))
                    {
                        *rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(1, 0), 1, inCellY);
                        *rightHeight += *rightAlteredHeight;
                    }
                }
            }
            if (inCellY == ESM::Land::LAND_SIZE - 1)
            {
                cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);
                if(!noDownCell && !noDownLand)
                {
                    const CSMWorld::LandHeightsColumn::DataType landDownShapePointer =
                        landTable.data(landTable.getModelIndex(cellDownId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    *downHeight = landDownShapePointer[ESM::Land::LAND_SIZE + inCellX];
                    if (paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1))
                    {
                        *downAlteredHeight = *paged->getCellAlteredHeight(cellCoords.move(0, 1), inCellX, 1);
                        *downHeight += *downAlteredHeight;
                    }
                }
            }

           //If not at edge, get values from the same cell
            if (inCellX != 0)
            {
                *leftHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX - 1];
                if (paged->getCellAlteredHeight(cellCoords, inCellX - 1, inCellY))
                    *leftAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX - 1, inCellY);
                *leftHeight += *leftAlteredHeight;
            }
            if (inCellY != 0)
            {
                *upHeight = landShapePointer[(inCellY - 1) * ESM::Land::LAND_SIZE + inCellX];
                if (paged->getCellAlteredHeight(cellCoords, inCellX, inCellY - 1))
                    *upAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY - 1);
                *upHeight += *upAlteredHeight;
            }
            if (inCellX != ESM::Land::LAND_SIZE - 1)
            {
                *rightHeight = landShapePointer[inCellY * ESM::Land::LAND_SIZE + inCellX + 1];
                if (paged->getCellAlteredHeight(cellCoords, inCellX + 1, inCellY))
                    *rightAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX + 1, inCellY);
                *rightHeight += *rightAlteredHeight;
            }
            if (inCellY != ESM::Land::LAND_SIZE - 1)
            {
                *downHeight = landShapePointer[(inCellY + 1) * ESM::Land::LAND_SIZE + inCellX];
                if (paged->getCellAlteredHeight(cellCoords, inCellX, inCellY + 1))
                    *downAlteredHeight = *paged->getCellAlteredHeight(cellCoords, inCellX, inCellY + 1);
                *downHeight += *downAlteredHeight;
            }

        }
    }
}

void CSVRender::TerrainShapeMode::compareAndLimit(const CSMWorld::CellCoordinates& cellCoords, int inCellX, int inCellY, float* limitedAlteredHeightXAxis, float* limitedAlteredHeightYAxis, bool* steepnessIsWithinLimits)
{
    if (limitedAlteredHeightXAxis)
    {
        if (limitedAlteredHeightYAxis)
        {
            if(std::abs(*limitedAlteredHeightXAxis) >= std::abs(*limitedAlteredHeightYAxis))
            {
                alterHeight(cellCoords, inCellX, inCellY, *limitedAlteredHeightXAxis, false);
                *steepnessIsWithinLimits = false;
            }
            else
            {
                alterHeight(cellCoords, inCellX, inCellY, *limitedAlteredHeightYAxis, false);
                *steepnessIsWithinLimits = false;
            }
        }
        else
        {
            alterHeight(cellCoords, inCellX, inCellY, *limitedAlteredHeightXAxis, false);
            *steepnessIsWithinLimits = false;
        }
    }
    else if (limitedAlteredHeightYAxis)
    {
        alterHeight(cellCoords, inCellX, inCellY, *limitedAlteredHeightYAxis, false);
        *steepnessIsWithinLimits = false;
    }
}

bool CSVRender::TerrainShapeMode::limitAlteredHeights(const CSMWorld::CellCoordinates& cellCoords, bool reverseMode)
{
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (*document.getData().getTableModel(CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);

    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());

    int limitHeightChange = 1016.0f; // Limited by save format
    bool noCell = document.getData().getCells().searchId (cellId) == -1;
    bool noLand = document.getData().getLand().searchId (cellId) == -1;
    bool steepnessIsWithinLimits = true;

    if (!noCell && !noLand)
    {
        const CSMWorld::LandHeightsColumn::DataType landShapePointer =
            landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

        float thisHeight = 0.0f;
        float thisAlteredHeight = 0.0f;
        float leftHeight = 0.0f;
        float leftAlteredHeight = 0.0f;
        float upHeight = 0.0f;
        float upAlteredHeight = 0.0f;
        float rightHeight = 0.0f;
        float rightAlteredHeight = 0.0f;
        float downHeight = 0.0f;
        float downAlteredHeight = 0.0f;

        if (reverseMode == false)
        {
            for(int inCellY = 0; inCellY < ESM::Land::LAND_SIZE; ++inCellY)
            {
                for(int inCellX = 0; inCellX < ESM::Land::LAND_SIZE; ++inCellX)
                {
                    std::unique_ptr<float> limitedAlteredHeightXAxis(nullptr);
                    std::unique_ptr<float> limitedAlteredHeightYAxis(nullptr);
                    updateKeyHeightValues(cellCoords, inCellX, inCellY, &thisHeight, &thisAlteredHeight, &leftHeight, &leftAlteredHeight,
                        &upHeight, &upAlteredHeight, &rightHeight, &rightAlteredHeight, &downHeight, &downAlteredHeight);

                    // Check for height limits on x-axis
                    if (leftHeight - thisHeight > limitHeightChange)
                        limitedAlteredHeightXAxis.reset(new float(leftHeight - limitHeightChange - (thisHeight - thisAlteredHeight)));
                    else if (leftHeight - thisHeight < -limitHeightChange)
                        limitedAlteredHeightXAxis.reset(new float(leftHeight + limitHeightChange - (thisHeight - thisAlteredHeight)));

                    // Check for height limits on y-axis
                    if (upHeight - thisHeight > limitHeightChange)
                        limitedAlteredHeightYAxis.reset(new float(upHeight - limitHeightChange - (thisHeight - thisAlteredHeight)));
                    else if (upHeight - thisHeight < -limitHeightChange)
                        limitedAlteredHeightYAxis.reset(new float(upHeight + limitHeightChange - (thisHeight - thisAlteredHeight)));

                    // Limit altered height value based on x or y, whichever is the smallest
                    compareAndLimit(cellCoords, inCellX, inCellY, limitedAlteredHeightXAxis.get(), limitedAlteredHeightYAxis.get(), &steepnessIsWithinLimits);
                }
            }
        }

        if (reverseMode == true)
        {
            for(int inCellY = ESM::Land::LAND_SIZE - 1; inCellY >= 0; --inCellY)
            {
                for(int inCellX = ESM::Land::LAND_SIZE - 1; inCellX >= 0; --inCellX)
                {
                    std::unique_ptr<float> limitedAlteredHeightXAxis(nullptr);
                    std::unique_ptr<float> limitedAlteredHeightYAxis(nullptr);
                    updateKeyHeightValues(cellCoords, inCellX, inCellY, &thisHeight, &thisAlteredHeight, &leftHeight, &leftAlteredHeight,
                        &upHeight, &upAlteredHeight, &rightHeight, &rightAlteredHeight, &downHeight, &downAlteredHeight);

                    // Check for height limits on x-axis
                    if (rightHeight - thisHeight > limitHeightChange)
                        limitedAlteredHeightXAxis.reset(new float(rightHeight - limitHeightChange - (thisHeight - thisAlteredHeight)));
                    else if (rightHeight - thisHeight < -limitHeightChange)
                        limitedAlteredHeightXAxis.reset(new float(rightHeight + limitHeightChange - (thisHeight - thisAlteredHeight)));

                    // Check for height limits on y-axis
                    if (downHeight - thisHeight > limitHeightChange)
                        limitedAlteredHeightYAxis.reset(new float(downHeight - limitHeightChange - (thisHeight - thisAlteredHeight)));
                    else if (downHeight - thisHeight < -limitHeightChange)
                        limitedAlteredHeightYAxis.reset(new float(downHeight + limitHeightChange - (thisHeight - thisAlteredHeight)));

                    // Limit altered height value based on x or y, whichever is the smallest
                    compareAndLimit(cellCoords, inCellX, inCellY, limitedAlteredHeightXAxis.get(), limitedAlteredHeightYAxis.get(), &steepnessIsWithinLimits);
                }
            }
        }
    }
    return steepnessIsWithinLimits;
}

void CSVRender::TerrainShapeMode::selectTerrainShapes(const std::pair<int, int>& vertexCoords, unsigned char selectMode, bool dragOperation)
{
    int r = mBrushSize / 2;
    std::vector<std::pair<int, int>> selections;

    if (mBrushShape == CSVWidget::BrushShape_Point)
    {
        selections.emplace_back(vertexCoords);
    }

    if (mBrushShape == CSVWidget::BrushShape_Square)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                selections.emplace_back(std::make_pair(i, j));
            }
        }
    }

    if (mBrushShape == CSVWidget::BrushShape_Circle)
    {
        for(int i = vertexCoords.first - r; i <= vertexCoords.first + r; ++i)
        {
            for(int j = vertexCoords.second - r; j <= vertexCoords.second + r; ++j)
            {
                int distanceX = abs(i - vertexCoords.first);
                int distanceY = abs(j - vertexCoords.second);
                int distance = std::round(sqrt(pow(distanceX, 2)+pow(distanceY, 2)));
                if (distance <= r) selections.emplace_back(std::make_pair(i, j));
            }
        }
    }

    if (mBrushShape == CSVWidget::BrushShape_Custom)
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

void CSVRender::TerrainShapeMode::pushEditToCommand(const CSMWorld::LandHeightsColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, const std::string& cellId)
{
    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandHeightsIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}

void CSVRender::TerrainShapeMode::pushNormalsEditToCommand(const CSMWorld::LandNormalsColumn::DataType& newLandGrid, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, const std::string& cellId)
{
    QVariant changedLand;
    changedLand.setValue(newLandGrid);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandNormalsIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}

void CSVRender::TerrainShapeMode::pushLodToCommand(const CSMWorld::LandMapLodColumn::DataType& newLandMapLod, CSMDoc::Document& document,
    CSMWorld::IdTable& landTable, const std::string& cellId)
{
    QVariant changedLod;
    changedLod.setValue(newLandMapLod);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandMapLodIndex)));

    QUndoStack& undoStack = document.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLod));
}

bool CSVRender::TerrainShapeMode::allowLandShapeEditing(const std::string& cellId)
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

void CSVRender::TerrainShapeMode::setBrushShape(CSVWidget::BrushShape brushShape)
{
    mBrushShape = brushShape;

    //Set custom brush shape
    if (mBrushShape == CSVWidget::BrushShape_Custom && !mTerrainShapeSelection->getTerrainSelection().empty())
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
