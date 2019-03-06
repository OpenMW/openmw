#include "scalingtool.hpp"

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUndoStack>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

#include "../doc/document.hpp"
#include "../world/idtable.hpp"
#include "../world/idtree.hpp"
#include "columnbase.hpp"
#include "commandmacro.hpp"
#include "commands.hpp"
#include "cellcoordinates.hpp"

CSMWorld::ScalingTool::ScalingTool(CSMDoc::Document& document, QWidget* parent) : QWidget(parent, Qt::Window)
    , mDocument(document)
{
    createSpinBoxes();

    setWindowTitle("Land scaling");

    QGroupBox *coordinateBoxes = new QGroupBox;
    QHBoxLayout *coordinateBoxesLayout = new QHBoxLayout;
    coordinateBoxesLayout->addWidget(mSourceGroupBox);
    coordinateBoxesLayout->addWidget(mTargetGroupBox);
    coordinateBoxes->setLayout(coordinateBoxesLayout);

    mHeightMethodSelector = new QComboBox(this);
    mHeightMethodSelector->addItem(tr("Bilinear interpolation (smooth)"));
    mHeightMethodSelector->addItem(tr("Nearest neighbor (fast)"));

    mTextureMethodSelector = new QComboBox(this);
    mTextureMethodSelector->addItem(tr("EPX (2x)"));
    mTextureMethodSelector->addItem(tr("Nearest neighbor (fast)"));

    mScaleZCheckBox = new QCheckBox("Scale Z", this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(coordinateBoxes);
    mainLayout->addWidget(mScaleZCheckBox);
    mainLayout->addWidget(mHeightMethodSelector);
    mainLayout->addWidget(mTextureMethodSelector);
    mainLayout->addWidget(mActionButton);
    setLayout(mainLayout);

    connect(mActionButton, SIGNAL(clicked()), this, SLOT(scaleNow()));
}

CSMWorld::ScalingTool::~ScalingTool()
{
}

void CSMWorld::ScalingTool::createSpinBoxes()
{
    mSourceGroupBox = new QGroupBox(tr("Source coordinates"));
    mTargetGroupBox = new QGroupBox(tr("Target coordinates"));

    mCellLabel_Source_CornerA = new QLabel(tr("Corner A of source, coordinates:"));

    mCellX_SpinBox_Source_CornerA = new QSpinBox;
    mCellX_SpinBox_Source_CornerA->setRange(-99999999, 99999999);
    mCellX_SpinBox_Source_CornerA->setValue(0);

    mCellY_SpinBox_Source_CornerA = new QSpinBox;
    mCellY_SpinBox_Source_CornerA->setRange(-99999999, 99999999);
    mCellY_SpinBox_Source_CornerA->setValue(0);

    mCellLabel_Source_CornerB = new QLabel(tr("Corner B of source, coordinates:"));

    mCellX_SpinBox_Source_CornerB = new QSpinBox;
    mCellX_SpinBox_Source_CornerB->setRange(-99999999, 99999999);
    mCellX_SpinBox_Source_CornerB->setValue(0);

    mCellY_SpinBox_Source_CornerB = new QSpinBox;
    mCellY_SpinBox_Source_CornerB->setRange(-99999999, 99999999);
    mCellY_SpinBox_Source_CornerB->setValue(0);

    mCellLabel_Target_CornerA = new QLabel(tr("Corner A of target, coordinates:"));

    mCellX_SpinBox_Target_CornerA = new QSpinBox;
    mCellX_SpinBox_Target_CornerA->setRange(-99999999, 99999999);
    mCellX_SpinBox_Target_CornerA->setValue(0);

    mCellY_SpinBox_Target_CornerA = new QSpinBox;
    mCellY_SpinBox_Target_CornerA->setRange(-99999999, 99999999);
    mCellY_SpinBox_Target_CornerA->setValue(0);

    mCellLabel_Target_CornerB = new QLabel(tr("Corner B of target, coordinates:"));

    mCellX_SpinBox_Target_CornerB = new QSpinBox;
    mCellX_SpinBox_Target_CornerB->setRange(-99999999, 99999999);
    mCellX_SpinBox_Target_CornerB->setValue(0);

    mCellY_SpinBox_Target_CornerB = new QSpinBox;
    mCellY_SpinBox_Target_CornerB->setRange(-99999999, 99999999);
    mCellY_SpinBox_Target_CornerB->setValue(0);

    mActionButton = new QPushButton("Scale!", this);

    QVBoxLayout *sourceSpinBoxLayout = new QVBoxLayout;
    sourceSpinBoxLayout->addWidget(mCellLabel_Source_CornerA);
    sourceSpinBoxLayout->addWidget(mCellX_SpinBox_Source_CornerA);
    sourceSpinBoxLayout->addWidget(mCellY_SpinBox_Source_CornerA);
    sourceSpinBoxLayout->addWidget(mCellLabel_Source_CornerB);
    sourceSpinBoxLayout->addWidget(mCellX_SpinBox_Source_CornerB);
    sourceSpinBoxLayout->addWidget(mCellY_SpinBox_Source_CornerB);

    QVBoxLayout *targetSpinBoxLayout = new QVBoxLayout;
    targetSpinBoxLayout->addWidget(mCellLabel_Target_CornerA);
    targetSpinBoxLayout->addWidget(mCellX_SpinBox_Target_CornerA);
    targetSpinBoxLayout->addWidget(mCellY_SpinBox_Target_CornerA);
    targetSpinBoxLayout->addWidget(mCellLabel_Target_CornerB);
    targetSpinBoxLayout->addWidget(mCellX_SpinBox_Target_CornerB);
    targetSpinBoxLayout->addWidget(mCellY_SpinBox_Target_CornerB);

    mSourceGroupBox->setLayout(sourceSpinBoxLayout);
    mTargetGroupBox->setLayout(targetSpinBoxLayout);

}

void CSMWorld::ScalingTool::scaleNow()
{
    scaleLand(mCellX_SpinBox_Source_CornerA->value(), mCellY_SpinBox_Source_CornerA->value(), mCellX_SpinBox_Source_CornerB->value(), mCellY_SpinBox_Source_CornerB->value(),
        mCellX_SpinBox_Target_CornerA->value(), mCellY_SpinBox_Target_CornerA->value(), mCellX_SpinBox_Target_CornerB->value(), mCellY_SpinBox_Target_CornerB->value());
}

void CSMWorld::ScalingTool::scaleLand(int cellX_Source_CornerA, int cellY_Source_CornerA, int cellX_Source_CornerB, int cellY_Source_CornerB,
    int cellX_Target_CornerA, int cellY_Target_CornerA, int cellX_Target_CornerB, int cellY_Target_CornerB)
{
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));
    CSMWorld::IdTree& cellTable = dynamic_cast<CSMWorld::IdTree&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));
    QUndoStack& undoStack = mDocument.getUndoStack();
    undoStack.beginMacro ("Scale land");

    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

    const int sourceSizeX = cellX_Source_CornerB - cellX_Source_CornerA + 1;
    const int sourceSizeY = cellY_Source_CornerB - cellY_Source_CornerA + 1;
    const int targetSizeX = cellX_Target_CornerB - cellX_Target_CornerA + 1;
    const int targetSizeY = cellY_Target_CornerB - cellY_Target_CornerA + 1;

    const int sourceVertexSizeX = sourceSizeX * (landSize - 1);
    const int sourceVertexSizeY = sourceSizeY * (landSize - 1);
    const int sourceTextureSizeX = sourceSizeX * landTextureSize;
    const int sourceTextureSizeY = sourceSizeY * landTextureSize;

    const int targetVertexSizeX = targetSizeX * (landSize - 1);
    const int targetVertexSizeY = targetSizeY * (landSize - 1);
    const int targetTextureSizeX = targetSizeX * landTextureSize;
    const int targetTextureSizeY = targetSizeY * landTextureSize;

    if (sourceSizeX == 0) return;
    if (sourceSizeY == 0) return;
    if (targetSizeX == 0) return;
    if (targetSizeY == 0) return;

    const int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const int landnormalsColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex);
    const int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

    //Scale heights and textures
    for (int newCellX = cellX_Target_CornerA; newCellX <= cellX_Target_CornerB; ++newCellX)
    {
        for (int newCellY = cellY_Target_CornerA; newCellY <= cellY_Target_CornerB; ++newCellY)
        {
            std::string newCellId = CSMWorld::CellCoordinates::generateId(newCellX, newCellY);

            // Create new cell and/or land if required
            bool noCell = mDocument.getData().getCells().searchId (newCellId)==-1;
            bool noLand = mDocument.getData().getLand().searchId (newCellId)==-1;
            if (noCell)
            {
                std::unique_ptr<CSMWorld::CreateCommand> createCommand (
                new CSMWorld::CreateCommand (cellTable, newCellId));
                int parentIndex = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
                int index = cellTable.findNestedColumnIndex (parentIndex, CSMWorld::Columns::ColumnId_Interior);
                createCommand->addNestedValue (parentIndex, index, false);
                mDocument.getUndoStack().push (createCommand.release());
            }
            if (noLand)
            {
                mDocument.getUndoStack().push (new CSMWorld::CreateCommand (landTable, newCellId));
            }

            //Get the old land height and make a copy. Old data of the target cell is not actually used, but similar data structure is required.
            CSMWorld::LandHeightsColumn::DataType oldHeightTarget = landTable.data(landTable.getModelIndex(newCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
            CSMWorld::LandHeightsColumn::DataType heightTarget(oldHeightTarget);

            // Go through every vertex in the cell, and find correct values from the source
            for (int xInTargetCell = 0; xInTargetCell < landSize; ++xInTargetCell)
            {
                for (int yInTargetCell = 0; yInTargetCell < landSize; ++yInTargetCell)
                {
                    float targetFractionX = static_cast<float>((newCellX - cellX_Target_CornerA) * (landSize - 1) + xInTargetCell) / targetVertexSizeX;
                    float targetFractionY = static_cast<float>((newCellY - cellY_Target_CornerA) * (landSize - 1) + yInTargetCell) / targetVertexSizeY;
                    float CoordinateOfSourceXTrue = targetFractionX * sourceVertexSizeX;
                    float CoordinateOfSourceYTrue = targetFractionY * sourceVertexSizeY;
                    int CoordinateOfSourceX0 = std::floor(CoordinateOfSourceXTrue);
                    int CoordinateOfSourceY0 = std::floor(CoordinateOfSourceYTrue);
                    int CoordinateOfSourceX1 = CoordinateOfSourceX0 + 1;
                    int CoordinateOfSourceY1 = CoordinateOfSourceY0 + 1;
                    int cellSourceX0 = (CoordinateOfSourceX0 / (landSize - 1)) + cellX_Source_CornerA;
                    int cellSourceY0 = (CoordinateOfSourceY0 / (landSize - 1)) + cellY_Source_CornerA;
                    int cellSourceX1 = (CoordinateOfSourceX1 / (landSize - 1)) + cellX_Source_CornerA;
                    int cellSourceY1 = (CoordinateOfSourceY1 / (landSize - 1)) + cellY_Source_CornerA;
                    int inCellSourceX0 = CoordinateOfSourceX0 - (CoordinateOfSourceX0 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceY0 = CoordinateOfSourceY0 - (CoordinateOfSourceY0 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceX1 = CoordinateOfSourceX1 - (CoordinateOfSourceX1 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceY1 = CoordinateOfSourceY1 - (CoordinateOfSourceY1 / (landSize - 1)) * (landSize - 1);

                    std::string sourceCellId = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY0);
                    std::string sourceCellId10 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY0);
                    std::string sourceCellId01 = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY1);
                    std::string sourceCellId11 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY1);

                    bool noSourceCell = mDocument.getData().getCells().searchId (sourceCellId)==-1;
                    bool noSourceLand = mDocument.getData().getLand().searchId (sourceCellId)==-1;

                    heightTarget[yInTargetCell * landSize + xInTargetCell] = 0;

                    if (!noSourceCell && !noSourceLand)
                    {
                        if(mHeightMethodSelector->currentText() == "Bilinear interpolation (smooth)")
                        {
                            //Use values of the same cell by default, only load heights at X1 and Y1 if there is land. TODO: Optimize and fix edges
                            CSMWorld::LandHeightsColumn::DataType heightSource00 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            CSMWorld::LandHeightsColumn::DataType heightSource10 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            CSMWorld::LandHeightsColumn::DataType heightSource01 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            CSMWorld::LandHeightsColumn::DataType heightSource11 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            if (mDocument.getData().getCells().searchId (sourceCellId10) != -1 && mDocument.getData().getLand().searchId (sourceCellId10) != -1)
                                heightSource10 = landTable.data(landTable.getModelIndex(sourceCellId10, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            if (mDocument.getData().getCells().searchId (sourceCellId01) != -1 && mDocument.getData().getLand().searchId (sourceCellId01) != -1)
                                heightSource01 = landTable.data(landTable.getModelIndex(sourceCellId01, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            if (mDocument.getData().getCells().searchId (sourceCellId11) != -1 && mDocument.getData().getLand().searchId (sourceCellId11) != -1)
                                heightSource11 = landTable.data(landTable.getModelIndex(sourceCellId11, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            float distFromX1 = CoordinateOfSourceX1 - CoordinateOfSourceXTrue;
                            float distFromX0 = 1 - distFromX1;
                            float distFromY1 = CoordinateOfSourceY1 - CoordinateOfSourceYTrue;
                            float distFromY0 = 1 - distFromY1;
                            float interpolatedXwhenY0 = heightSource00[inCellSourceY0 * landSize + inCellSourceX0] * distFromX1 + heightSource10[inCellSourceY0 * landSize + inCellSourceX1] * distFromX0;
                            float interpolatedXwhenY1 = heightSource01[inCellSourceY1 * landSize + inCellSourceX0] * distFromX1 + heightSource11[inCellSourceY1 * landSize + inCellSourceX1] * distFromX0;
                            heightTarget[yInTargetCell * landSize + xInTargetCell] = interpolatedXwhenY0 * distFromY1 + interpolatedXwhenY1 * distFromY0;

                            if(mScaleZCheckBox->checkState())
                            {
                                float scaleFactor = static_cast<float>(targetSizeX) / sourceSizeX;
                                heightTarget[yInTargetCell * landSize + xInTargetCell] *= scaleFactor;
                            }
                        }

                        if(mHeightMethodSelector->currentText() == "Nearest neighbor (fast)")
                        {
                            // Floored neighbor
                            CSMWorld::LandHeightsColumn::DataType heightSource00 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            heightTarget[yInTargetCell * landSize + xInTargetCell] = heightSource00[inCellSourceY0 * landSize + inCellSourceX0];

                            if(mScaleZCheckBox->checkState())
                            {
                                float scaleFactor = static_cast<float>(targetSizeX) / sourceSizeX;
                                heightTarget[yInTargetCell * landSize + xInTargetCell] *= scaleFactor;
                            }
                        }
                    }
                    //Land heights must go in steps of 8 to limit land tearing because of rounding errors
                    heightTarget[yInTargetCell * landSize + xInTargetCell] -= fmod(heightTarget[yInTargetCell * landSize + xInTargetCell], 8);
                }
            }

            //Get the old land texture data and make a copy. Old data of the target cell is not actually used, but similar data structure is required.
            CSMWorld::LandTexturesColumn::DataType oldTexTarget = landTable.data(landTable.getModelIndex(newCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
            CSMWorld::LandTexturesColumn::DataType texTarget(oldTexTarget);
            for (int xInTargetCell = 0; xInTargetCell < landTextureSize; ++xInTargetCell)
            {
                for (int yInTargetCell = 0; yInTargetCell < landTextureSize; ++yInTargetCell)
                {
                    float targetFractionX = static_cast<float>((newCellX - cellX_Target_CornerA) * landTextureSize + xInTargetCell) / targetTextureSizeX;
                    float targetFractionY = static_cast<float>((newCellY - cellY_Target_CornerA) * landTextureSize + yInTargetCell) / targetTextureSizeY;
                    float CoordinateOfSourceXTrue = targetFractionX * sourceTextureSizeX;
                    float CoordinateOfSourceYTrue = targetFractionY * sourceTextureSizeY;
                    int CoordinateOfSourceX0 = std::floor(CoordinateOfSourceXTrue);
                    int CoordinateOfSourceY0 = std::floor(CoordinateOfSourceYTrue);
                    int cellSourceX0 = (CoordinateOfSourceX0 / landTextureSize) + cellX_Source_CornerA;
                    int cellSourceY0 = (CoordinateOfSourceY0 / landTextureSize) + cellY_Source_CornerA;
                    int inCellSourceX0 = CoordinateOfSourceX0 - (CoordinateOfSourceX0 / landTextureSize) * landTextureSize;
                    int inCellSourceY0 = CoordinateOfSourceY0 - (CoordinateOfSourceY0 / landTextureSize) * landTextureSize;

                    std::string sourceCellId = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY0);

                    bool noSourceCell = mDocument.getData().getCells().searchId (sourceCellId)==-1;
                    bool noSourceLand = mDocument.getData().getLand().searchId (sourceCellId)==-1;

                    if (!noSourceCell && !noSourceLand)
                    {
                        if(mTextureMethodSelector->currentText() == "Nearest neighbor (fast)")
                        {
                            // Floored neighbor
                            CSMWorld::LandTexturesColumn::DataType texSource00 = landTable.data(landTable.getModelIndex(sourceCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                            texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource00[inCellSourceY0 * landTextureSize + inCellSourceX0];
                        }
                        if(mTextureMethodSelector->currentText() == "EPX (2x)")
                        {
                            int CoordinateOfSourceXMinus1 = CoordinateOfSourceX0 - 1;
                            int CoordinateOfSourceYMinus1 = CoordinateOfSourceY0 - 1;
                            int CoordinateOfSourceX1 = CoordinateOfSourceX0 + 1;
                            int CoordinateOfSourceY1 = CoordinateOfSourceY0 + 1;
                            int inCellSourceXMinus1 = CoordinateOfSourceXMinus1 - (CoordinateOfSourceXMinus1 / landTextureSize) * landTextureSize;
                            int inCellSourceYMinus1 = CoordinateOfSourceYMinus1 - (CoordinateOfSourceYMinus1 / landTextureSize) * landTextureSize;
                            int inCellSourceX1 = CoordinateOfSourceX1 - (CoordinateOfSourceX1 / landTextureSize) * landTextureSize;
                            int inCellSourceY1 = CoordinateOfSourceY1 - (CoordinateOfSourceY1 / landTextureSize) * landTextureSize;
                            int cellSourceXMinus1 = (CoordinateOfSourceXMinus1 / landTextureSize) + cellX_Source_CornerA;
                            int cellSourceYMinus1 = (CoordinateOfSourceYMinus1 / landTextureSize) + cellY_Source_CornerA;
                            int cellSourceX1 = (CoordinateOfSourceX1 / landTextureSize) + cellX_Source_CornerA;
                            int cellSourceY1 = (CoordinateOfSourceY1 / landTextureSize) + cellY_Source_CornerA;
                            std::string sourceCellIdMinus10 = CSMWorld::CellCoordinates::generateId(cellSourceXMinus1, cellSourceY0);
                            std::string sourceCellId0Minus1 = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceYMinus1);
                            std::string sourceCellId10 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY0);
                            std::string sourceCellId01 = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY1);

                            float distFromX1 = CoordinateOfSourceX1 - CoordinateOfSourceXTrue;
                            float distFromY1 = CoordinateOfSourceY1 - CoordinateOfSourceYTrue;

                            CSMWorld::LandTexturesColumn::DataType texSource00 = landTable.data(landTable.getModelIndex(sourceCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                            CSMWorld::LandTexturesColumn::DataType texSourceMinus10 = landTable.data(landTable.getModelIndex(sourceCellIdMinus10, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                            CSMWorld::LandTexturesColumn::DataType texSource0Minus1 = landTable.data(landTable.getModelIndex(sourceCellId0Minus1, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                            CSMWorld::LandTexturesColumn::DataType texSource10 = landTable.data(landTable.getModelIndex(sourceCellId10, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                            CSMWorld::LandTexturesColumn::DataType texSource01 = landTable.data(landTable.getModelIndex(sourceCellId01, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();

                            texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource00[inCellSourceY0 * landTextureSize + inCellSourceX0];
                            int aTex = texSource0Minus1[inCellSourceYMinus1 * landTextureSize + inCellSourceX0];
                            int bTex = texSource10[inCellSourceY0 * landTextureSize + inCellSourceX1];
                            int cTex = texSourceMinus10[inCellSourceY0 * landTextureSize + inCellSourceXMinus1];
                            int dTex = texSource01[inCellSourceY1 * landTextureSize + inCellSourceX0];
                            if (cTex == aTex &&
                                distFromX1 > 0.5 && distFromY1 > 0.5)
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = aTex;
                            if (aTex == bTex &&
                                distFromX1 <= 0.5 && distFromY1 > 0.5)
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = bTex;
                            if (dTex == cTex &&
                                distFromX1 > 0.5 && distFromY1 <= 0.5)
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = cTex;
                            if (bTex == dTex &&
                                distFromX1 <= 0.5 && distFromY1 <= 0.5)
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = dTex;
                            if ((aTex == bTex && bTex == cTex) ||
                                (aTex == bTex && bTex == dTex) ||
                                (aTex == cTex && cTex == dTex) ||
                                (bTex == cTex && cTex == dTex))
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource00[inCellSourceY0 * landTextureSize + inCellSourceX0];
                        }
                    }
                    else
                    {
                        texTarget[yInTargetCell * landTextureSize + xInTargetCell] = 0;
                    }
                }
            }

            // Commands that push pending changes to the actual data
            QVariant changedLandTex;
            QVariant changedLandHeight;
            changedLandHeight.setValue(heightTarget);
            changedLandTex.setValue(texTarget);
            QModelIndex Qindex(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandHeightsIndex)));
            QModelIndex QindexTex(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandTexturesIndex)));
            undoStack.push (new CSMWorld::ModifyCommand(landTable, Qindex, changedLandHeight));
            undoStack.push (new CSMWorld::ModifyCommand(landTable, QindexTex, changedLandTex));
            undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, newCellId)); //Touch command gets correct texture indexes to cell
        }
    }

    //Generate normals
    for (int newCellX = cellX_Target_CornerA; newCellX <= cellX_Target_CornerB; ++newCellX)
    {
        for (int newCellY = cellY_Target_CornerA; newCellY <= cellY_Target_CornerB; ++newCellY)
        {

            // rightCell and downCell means one vertex right or down, usually in the same cell
            std::string newCellId = CSMWorld::CellCoordinates::generateId(newCellX, newCellY);
            std::string rightCellId;
            std::string downCellId;

            //Get the old land normals data and make a copy. Old data of the target cell is not actually used, but similar data structure is required.
            const CSMWorld::LandHeightsColumn::DataType heightSource00 = landTable.data(landTable.getModelIndex(newCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
            CSMWorld::LandHeightsColumn::DataType heightSource10;
            CSMWorld::LandHeightsColumn::DataType heightSource01;// = landTable.data(landTable.getModelIndex(downCellId), landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
            const CSMWorld::LandNormalsColumn::DataType oldNormalsTarget = landTable.data(landTable.getModelIndex(newCellId, landnormalsColumn)).value<CSMWorld::LandNormalsColumn::DataType>();
            CSMWorld::LandNormalsColumn::DataType normalsTarget(oldNormalsTarget);

            for(int i = 0; i < landSize; ++i)
            {
                if (i == landSize - 1)
                {
                    rightCellId = CSMWorld::CellCoordinates::generateId(newCellX + 1, newCellY);
                    heightSource10 = landTable.data(landTable.getModelIndex(rightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                }
                else
                {
                    rightCellId = CSMWorld::CellCoordinates::generateId(newCellX, newCellY);
                    heightSource10 = landTable.data(landTable.getModelIndex(rightCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                }

                for(int j = 0; j < landSize; ++j)
                {
                    if (j == landSize - 1)
                    {
                        downCellId = CSMWorld::CellCoordinates::generateId(newCellX, newCellY + 1);
                        heightSource01 = landTable.data(landTable.getModelIndex(downCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    }
                    else
                    {
                        downCellId = CSMWorld::CellCoordinates::generateId(newCellX, newCellY);
                        heightSource01 = landTable.data(landTable.getModelIndex(downCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                    }

                    float v1[3];
                    float v2[3];
                    float normal[3];
                    float hyp;

                    v1[0] = 128;
                    v1[1] = 0;
                    if (i < landSize - 1) v1[2] = heightSource00[j*landSize+i+1] - heightSource00[j*landSize+i];
                    else v1[2] = heightSource10[j*landSize+1] - heightSource00[j*landSize+i];

                    v2[0] = 0;
                    v2[1] = 128;
                    if (j < landSize - 1) v2[2] = heightSource00[(j+1)*landSize+i] - heightSource00[j*landSize+i];
                    else v2[2] = heightSource01[landSize+i] - heightSource00[j*landSize+i];

                    normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
                    normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
                    normal[2] = v1[0]*v2[1] - v1[1]*v2[0];

                    hyp = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]) / 127.0f;

                    normal[0] /= hyp;
                    normal[1] /= hyp;
                    normal[2] /= hyp;

                    normalsTarget[(j*landSize+i)*3+0] = normal[0];
                    normalsTarget[(j*landSize+i)*3+1] = normal[1];
                    normalsTarget[(j*landSize+i)*3+2] = normal[2];
                }
            }

            //Push normals to data
            QVariant changedLandNormals;
            changedLandNormals.setValue(normalsTarget);
            QModelIndex indexNormals(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandNormalsIndex)));
            undoStack.push (new CSMWorld::ModifyCommand(landTable, indexNormals, changedLandNormals));
        }
    }
    undoStack.endMacro ();
}
