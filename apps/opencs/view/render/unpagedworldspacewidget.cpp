
#include "unpagedworldspacewidget.hpp"

#include <OgreColourValue.h>

#include <QtGui/qevent.h>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../widget/scenetooltoggle.hpp"

#include "elements.hpp"

void CSVRender::UnpagedWorldspaceWidget::update()
{
    const CSMWorld::Record<CSMWorld::Cell>& record =
        dynamic_cast<const CSMWorld::Record<CSMWorld::Cell>&> (mCellsModel->getRecord (mCellId));

    Ogre::ColourValue colour;
    colour.setAsABGR (record.get().mAmbi.mAmbient);
    setDefaultAmbient (colour);

    /// \todo deal with mSunlight and mFog/mForDensity

    flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle *tool)
{
    WorldspaceWidget::addVisibilitySelectorButtons (tool);

    tool->addButton (":armor.png", Element_Fog, ":armor.png", "Fog");
}

CSVRender::UnpagedWorldspaceWidget::UnpagedWorldspaceWidget (const std::string& cellId, CSMDoc::Document& document, QWidget* parent)
: WorldspaceWidget (document, parent), mCellId (cellId)
{
    mCellsModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    mReferenceablesModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    connect (mCellsModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mCellsModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRowsAboutToBeRemoved (const QModelIndex&, int, int)));

    update();

    mCell.reset (new Cell (document.getData(), getSceneManager(), mCellId));
}

void CSVRender::UnpagedWorldspaceWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    int index = mCellsModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification);
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, index);

    if (cellIndex.row()>=topLeft.row() && cellIndex.row()<=bottomRight.row())
    {
        if (mCellsModel->data (cellIndex).toInt()==CSMWorld::RecordBase::State_Deleted)
        {
            emit closeRequest();
        }
        else
        {
            /// \todo possible optimisation: check columns and update only if relevant columns have
            /// changed
            update();
        }
    }
}

void CSVRender::UnpagedWorldspaceWidget::cellRowsAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, 0);

    if (cellIndex.row()>=start && cellIndex.row()<=end)
        emit closeRequest();
}

void CSVRender::UnpagedWorldspaceWidget::handleDrop (const std::vector< CSMWorld::UniversalId >& data)
{
    mCellId = data.begin()->getId();
    update();
    emit cellChanged(*data.begin());

    /// \todo replace mCell
}

void CSVRender::UnpagedWorldspaceWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
    if (mCell.get())
    {
        QModelIndex topLeft = mReferenceablesModel->index (start, 0);
        QModelIndex bottomRight =
            mReferenceablesModel->index (end, mReferenceablesModel->columnCount());

        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
}

void CSVRender::UnpagedWorldspaceWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAdded (parent, start, end))
            flagAsModified();
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::UnpagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::dropType type) const
{
    switch(type)
    {
        case cellsInterior:
            return canHandle;

        case cellsExterior:
            return needPaged;

        default:
            return ignored;
    }
}
