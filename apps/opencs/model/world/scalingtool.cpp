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

    mMethodSelector = new QComboBox(this);
    mMethodSelector->addItem(tr("Bilinear interpolation (best)"));
    mMethodSelector->addItem(tr("Nearest neighbor (simple)"));

    mScaleZCheckBox = new QCheckBox("Scale Z", this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(coordinateBoxes);
    mainLayout->addWidget(mScaleZCheckBox);
    mainLayout->addWidget(mMethodSelector);
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

    const int sourceSizeX = cellX_Source_CornerB - cellX_Source_CornerA + 1;
    const int sourceSizeY = cellY_Source_CornerB - cellY_Source_CornerA + 1;
    const int targetSizeX = cellX_Target_CornerB - cellX_Target_CornerA + 1;
    const int targetSizeY = cellY_Target_CornerB - cellY_Target_CornerA + 1;

    const int landSize {ESM::Land::LAND_SIZE};
    const int landTextureSize {ESM::Land::LAND_TEXTURE_SIZE};

    const int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    const int landnormalsColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandNormalsIndex);
    const int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);

    //Go through all cells of the scaled land (target)
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
                    float xPercentage = static_cast<float>((newCellX - cellX_Target_CornerA) * (landSize - 1) + xInTargetCell) / (targetSizeX * landSize - 1);
                    float yPercentage = static_cast<float>((newCellY - cellY_Target_CornerA) * (landSize - 1) + yInTargetCell) / (targetSizeY * landSize - 1);
                    int xTrueSource0 = xPercentage * sourceSizeX * (landSize - 1);
                    int yTrueSource0 = yPercentage * sourceSizeY * (landSize - 1);
                    int xTrueSource1 = xPercentage * sourceSizeX * (landSize - 1) + 1;
                    int yTrueSource1 = yPercentage * sourceSizeY * (landSize - 1) + 1;
                    int cellSourceX0 = (xTrueSource0 / (landSize - 1)) + cellX_Source_CornerA;
                    int cellSourceY0 = (yTrueSource0 / (landSize - 1)) + cellY_Source_CornerA;
                    int cellSourceX1 = (xTrueSource1 / (landSize - 1)) + cellX_Source_CornerA;
                    int cellSourceY1 = (yTrueSource1 / (landSize - 1)) + cellY_Source_CornerA;
                    int inCellSourceX0 = xTrueSource0 - (xTrueSource0 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceY0 = yTrueSource0 - (yTrueSource0 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceX1 = xTrueSource1 - (xTrueSource1 / (landSize - 1)) * (landSize - 1);
                    int inCellSourceY1 = yTrueSource1 - (yTrueSource1 / (landSize - 1)) * (landSize - 1);

                    std::string sourceCellId = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY0);
                    std::string sourceCellId10 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY0);
                    std::string sourceCellId01 = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY1);
                    std::string sourceCellId11 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY1);

                    bool noSourceCell = mDocument.getData().getCells().searchId (sourceCellId)==-1;
                    bool noSourceLand = mDocument.getData().getLand().searchId (sourceCellId)==-1;

                    heightTarget[yInTargetCell * landSize + xInTargetCell] = 0;

                    if (!noSourceCell && !noSourceLand)
                    {
                        if(mMethodSelector->currentText() == "Bilinear interpolation (best)")
                        {
                            //Use X0Y0 by default, only load heights at X1 and Y1 if there is land. TODO: Optimize
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
                            float distFromX1 = xTrueSource1 - (xPercentage * sourceSizeX * (landSize - 1));
                            float distFromX0 = 1 - distFromX1;
                            float distFromY1 = yTrueSource1 - (yPercentage * sourceSizeY * (landSize - 1));
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

                        if(mMethodSelector->currentText() == "Nearest neighbor (simple)")
                        {
                            float distFromX1 = xTrueSource1 - (xPercentage * sourceSizeX * (landSize - 1));
                            float distFromY1 = yTrueSource1 - (yPercentage * sourceSizeY * (landSize - 1));

                            // The default nearest is always X0Y0, guaranteed to have cell and land
                            CSMWorld::LandHeightsColumn::DataType heightSource00 = landTable.data(landTable.getModelIndex(sourceCellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                            heightTarget[yInTargetCell * landSize + xInTargetCell] = heightSource00[inCellSourceY0 * landSize + inCellSourceX0];

                            // In case actual nearest is at X1 or Y1, update the value only if there is cell & land
                            if (distFromX1 <= 0.5 && distFromY1 > 0.5)
                            {
                                if (mDocument.getData().getCells().searchId (sourceCellId10) != -1 && mDocument.getData().getLand().searchId (sourceCellId10) != -1)
                                {
                                    CSMWorld::LandHeightsColumn::DataType heightSource = landTable.data(landTable.getModelIndex(sourceCellId10, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                                    heightTarget[yInTargetCell * landSize + xInTargetCell] = heightSource[inCellSourceY0 * landSize + inCellSourceX1];
                                }
                            }
                            else if (distFromX1 > 0.5 && distFromY1 <= 0.5)
                            {
                                if (mDocument.getData().getCells().searchId (sourceCellId01) != -1 && mDocument.getData().getLand().searchId (sourceCellId01) != -1)
                                {
                                    CSMWorld::LandHeightsColumn::DataType heightSource = landTable.data(landTable.getModelIndex(sourceCellId01, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                                    heightTarget[yInTargetCell * landSize + xInTargetCell] = heightSource[inCellSourceY1 * landSize + inCellSourceX0];
                                }
                            }
                            else if (distFromX1 <= 0.5 && distFromY1 <= 0.5)
                            {
                                if (mDocument.getData().getCells().searchId (sourceCellId11) != -1 && mDocument.getData().getLand().searchId (sourceCellId11) != -1)
                                {
                                    CSMWorld::LandHeightsColumn::DataType heightSource = landTable.data(landTable.getModelIndex(sourceCellId11, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
                                    heightTarget[yInTargetCell * landSize + xInTargetCell] = heightSource[inCellSourceY1 * landSize + inCellSourceX1];
                                }
                            }

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
                    float xPercentage = static_cast<float>((newCellX - cellX_Target_CornerA) * landTextureSize + xInTargetCell) / (targetSizeX * landTextureSize);
                    float yPercentage = static_cast<float>((newCellY - cellY_Target_CornerA) * landTextureSize + yInTargetCell) / (targetSizeY * landTextureSize);
                    int xTrueSource0 = xPercentage * sourceSizeX * landTextureSize;
                    int yTrueSource0 = yPercentage * sourceSizeY * landTextureSize;
                    int xTrueSource1 = xPercentage * sourceSizeX * landTextureSize + 1;
                    int yTrueSource1 = yPercentage * sourceSizeY * landTextureSize + 1;
                    int cellSourceX0 = (xTrueSource0 / landTextureSize) + cellX_Source_CornerA;
                    int cellSourceY0 = (yTrueSource0 / landTextureSize) + cellY_Source_CornerA;
                    int cellSourceX1 = (xTrueSource1 / landTextureSize) + cellX_Source_CornerA;
                    int cellSourceY1 = (yTrueSource1 / landTextureSize) + cellY_Source_CornerA;
                    int inCellSourceX0 = xTrueSource0 - (xTrueSource0 / landTextureSize) * landTextureSize;
                    int inCellSourceY0 = yTrueSource0 - (yTrueSource0 / landTextureSize) * landTextureSize;
                    int inCellSourceX1 = xTrueSource1 - (xTrueSource1 / landTextureSize) * landTextureSize;
                    int inCellSourceY1 = yTrueSource1 - (yTrueSource1 / landTextureSize) * landTextureSize;

                    std::string sourceCellId = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY0);
                    std::string sourceCellId10 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY0);
                    std::string sourceCellId01 = CSMWorld::CellCoordinates::generateId(cellSourceX0, cellSourceY1);
                    std::string sourceCellId11 = CSMWorld::CellCoordinates::generateId(cellSourceX1, cellSourceY1);

                    bool noSourceCell = mDocument.getData().getCells().searchId (sourceCellId)==-1;
                    bool noSourceLand = mDocument.getData().getLand().searchId (sourceCellId)==-1;

                    if (!noSourceCell && !noSourceLand)
                    {
                        float distFromX1 = xTrueSource1 - (xPercentage * sourceSizeX * landTextureSize);
                        float distFromY1 = yTrueSource1 - (yPercentage * sourceSizeY * landTextureSize);

                        // The default nearest is always X0Y0, guaranteed to have cell and land
                        CSMWorld::LandTexturesColumn::DataType texSource00 = landTable.data(landTable.getModelIndex(sourceCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                        texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource00[inCellSourceY0 * landTextureSize + inCellSourceX0];

                        // In case actual nearest is at X1 or Y1, update the value only if there is cell & land
                        if (distFromX1 <= 0.5 && distFromY1 > 0.5)
                        {
                            if (mDocument.getData().getCells().searchId (sourceCellId10) != -1 && mDocument.getData().getLand().searchId (sourceCellId10) != -1)
                            {
                                CSMWorld::LandTexturesColumn::DataType texSource10 = landTable.data(landTable.getModelIndex(sourceCellId10, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource10[inCellSourceY0 * landTextureSize + inCellSourceX1];
                            }
                        }
                        else if (distFromX1 > 0.5 && distFromY1 <= 0.5)
                        {
                            if (mDocument.getData().getCells().searchId (sourceCellId01) != -1 && mDocument.getData().getLand().searchId (sourceCellId01) != -1)
                            {
                                CSMWorld::LandTexturesColumn::DataType texSource01 = landTable.data(landTable.getModelIndex(sourceCellId01, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource01[inCellSourceY1 * landTextureSize + inCellSourceX0];
                            }
                        }
                        else if (distFromX1 <= 0.5 && distFromY1 <= 0.5)
                        {
                            if (mDocument.getData().getCells().searchId (sourceCellId11) != -1 && mDocument.getData().getLand().searchId (sourceCellId11) != -1)
                            {
                                CSMWorld::LandTexturesColumn::DataType texSource11 = landTable.data(landTable.getModelIndex(sourceCellId11, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
                                texTarget[yInTargetCell * landTextureSize + xInTargetCell] = texSource11[inCellSourceY1 * landTextureSize + inCellSourceX1];
                            }
                        }
                    }
                    else
                    {
                        texTarget[yInTargetCell * landTextureSize + xInTargetCell] = 0;
                    }
                }
            }

            //Get the old land normals data and make a copy. Old data of the target cell is not actually used, but similar data structure is required.
            CSMWorld::LandNormalsColumn::DataType oldNormalsTarget = landTable.data(landTable.getModelIndex(newCellId, landnormalsColumn)).value<CSMWorld::LandNormalsColumn::DataType>();
            CSMWorld::LandNormalsColumn::DataType normalsTarget(oldNormalsTarget);

            float v1[3],
                v2[3],
                normal[3],
                hyp;
            for(int i = 0; i < landSize - 1; i++)
            {
                for(int j = 0; j < landSize - 1; j++)
                {
                    //Generate new normals, code modified from tesanwynn
                    v1[0] = 128;
                    v1[1] = 0;
                    v1[2] = heightTarget[j*landSize+i+1] - heightTarget[j*landSize+i];

                    v2[0] = 0;
                    v2[1] = 128;
                    v2[2] = heightTarget[(j+1)*landSize+i] - heightTarget[j*landSize+i];

                    normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
                    normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
                    normal[2] = v1[0]*v2[1] - v1[1]*v2[0];

                    hyp = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]) / 127.0f;

                    normal[0] /= hyp; normal[1] /= hyp; normal[2] /= hyp;
                    normalsTarget[(j*landSize+i)*3+0] = normal[0];
                    normalsTarget[(j*landSize+i)*3+1] = normal[1];
                    normalsTarget[(j*landSize+i)*3+2] = normal[2];
                }
            }

            // Commands that push pending changes to the actual data
            QVariant changedLandTex;
            QVariant changedLandHeight;
            QVariant changedLandNormals;
            changedLandHeight.setValue(heightTarget);
            changedLandTex.setValue(texTarget);
            changedLandNormals.setValue(normalsTarget);
            QModelIndex Qindex(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandHeightsIndex)));
            QModelIndex QindexTex(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandTexturesIndex)));
            QModelIndex indexNormals(landTable.getModelIndex (newCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandNormalsIndex)));
            undoStack.push (new CSMWorld::ModifyCommand(landTable, Qindex, changedLandHeight));
            undoStack.push (new CSMWorld::ModifyCommand(landTable, QindexTex, changedLandTex));
            undoStack.push (new CSMWorld::TouchLandCommand(landTable, ltexTable, newCellId)); //Touch command gets correct texture indexes to cell
            undoStack.push (new CSMWorld::ModifyCommand(landTable, indexNormals, changedLandNormals));
        }
    }
    undoStack.endMacro ();
}
