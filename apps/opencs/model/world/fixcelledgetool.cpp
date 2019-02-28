#include "fixcelledgetool.hpp"

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUndoStack>
#include <QPushButton>

#include "../doc/document.hpp"
#include "../world/idtable.hpp"
#include "columnbase.hpp"
#include "commandmacro.hpp"
#include "commands.hpp"
#include "cellcoordinates.hpp"

CSMWorld::FixCellEdgeTool::FixCellEdgeTool(CSMDoc::Document& document, QWidget* parent) : QWidget(parent, Qt::Window)
    , mDocument(document)
{
    createInterface();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(mSpinBoxGroup);
    setLayout(layout);

    connect(mActionButton, SIGNAL(clicked()), this, SLOT(fixNow()));
}

CSMWorld::FixCellEdgeTool::~FixCellEdgeTool()
{
}

void CSMWorld::FixCellEdgeTool::createInterface()
{
    mSpinBoxGroup = new QGroupBox(tr("Cell coordinates"));

    mFixCellLabel = new QLabel(tr("Fix land heights at cell edges:"));

    mCellXSpinBox = new QSpinBox;
    mCellXSpinBox->setRange(-99999999, 99999999);
    mCellXSpinBox->setValue(0);


    mCellYSpinBox = new QSpinBox;
    mCellYSpinBox->setRange(-99999999, 99999999);
    mCellYSpinBox->setValue(0);

    mActionButton = new QPushButton("Fix!", this);

    QVBoxLayout *spinBoxLayout = new QVBoxLayout;
    spinBoxLayout->addWidget(mFixCellLabel);
    spinBoxLayout->addWidget(mCellXSpinBox);
    spinBoxLayout->addWidget(mCellYSpinBox);
    spinBoxLayout->addWidget(mActionButton);
    mSpinBoxGroup->setLayout(spinBoxLayout);
}

void CSMWorld::FixCellEdgeTool::fixNow()
{
    fixEdges(CSMWorld::CellCoordinates(mCellXSpinBox->value(), mCellYSpinBox->value()));
}

void CSMWorld::FixCellEdgeTool::fixEdges(CSMWorld::CellCoordinates cellCoords)
{
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    int landshapeColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandHeightsIndex);
    std::string cellId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY());
    std::string cellLeftId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() - 1, cellCoords.getY());
    std::string cellRightId = CSMWorld::CellCoordinates::generateId(cellCoords.getX() + 1, cellCoords.getY());
    std::string cellUpId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() - 1);
    std::string cellDownId = CSMWorld::CellCoordinates::generateId(cellCoords.getX(), cellCoords.getY() + 1);

    const CSMWorld::LandHeightsColumn::DataType landShapePointer = landTable.data(landTable.getModelIndex(cellId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    const CSMWorld::LandHeightsColumn::DataType landLeftShapePointer = landTable.data(landTable.getModelIndex(cellLeftId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    const CSMWorld::LandHeightsColumn::DataType landRightShapePointer = landTable.data(landTable.getModelIndex(cellRightId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    const CSMWorld::LandHeightsColumn::DataType landUpShapePointer = landTable.data(landTable.getModelIndex(cellUpId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();
    const CSMWorld::LandHeightsColumn::DataType landDownShapePointer = landTable.data(landTable.getModelIndex(cellDownId, landshapeColumn)).value<CSMWorld::LandHeightsColumn::DataType>();

    CSMWorld::LandHeightsColumn::DataType landShapeNew(landShapePointer);
    for(int i = 0; i < landSize; ++i)
    {
        if(mDocument.getData().getCells().searchId (cellLeftId) != -1 && mDocument.getData().getLand().searchId (cellLeftId) != -1 && landShapePointer[i * landSize] != landLeftShapePointer[i * landSize + landSize - 1]) landShapeNew[i * landSize] = landLeftShapePointer[i * landSize + landSize - 1];
        if(mDocument.getData().getCells().searchId (cellRightId) != -1 && mDocument.getData().getLand().searchId (cellRightId) != -1 && landShapePointer[i * landSize + landSize - 1] != landRightShapePointer[i * landSize]) landShapeNew[i * landSize + landSize - 1] = landRightShapePointer[i * landSize];
        if(mDocument.getData().getCells().searchId (cellUpId) != -1 && mDocument.getData().getLand().searchId (cellUpId) != -1 && landShapePointer[i] != landUpShapePointer[(landSize - 1) * landSize + i]) landShapeNew[i] = landUpShapePointer[(landSize - 1) * landSize + i];
        if(mDocument.getData().getCells().searchId (cellDownId) != -1 && mDocument.getData().getLand().searchId (cellDownId) != -1 && landShapePointer[(landSize - 1) * landSize + i] != landDownShapePointer[i]) landShapeNew[(landSize - 1) * landSize + i] = landDownShapePointer[i];
    }

    QVariant changedLand;
    changedLand.setValue(landShapeNew);

    QModelIndex index(landTable.getModelIndex (cellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandHeightsIndex)));
    QUndoStack& undoStack = mDocument.getUndoStack();
    undoStack.push (new CSMWorld::ModifyCommand(landTable, index, changedLand));
}
