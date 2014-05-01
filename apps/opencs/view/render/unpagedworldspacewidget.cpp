
#include "unpagedworldspacewidget.hpp"

#include <OgreColourValue.h>

#include <qt4/QtGui/qevent.h>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"

void CSVRender::UnpagedWorldspaceWidget::update()
{
    const CSMWorld::Record<CSMWorld::Cell>& record =
        dynamic_cast<const CSMWorld::Record<CSMWorld::Cell>&> (mCellsModel->getRecord (mCellId));

    Ogre::ColourValue colour;
    colour.setAsABGR (record.get().mAmbi.mAmbient);
    setDefaultAmbient (colour);

    /// \todo deal with mSunlight and mFog/mForDensity
}

CSVRender::UnpagedWorldspaceWidget::UnpagedWorldspaceWidget (const std::string& cellId, CSMDoc::Document& document, QWidget* parent)
: WorldspaceWidget (document, parent), mCellId (cellId)
{
    mCellsModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    connect (mCellsModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mCellsModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRowsAboutToBeRemoved (const QModelIndex&, int, int)));

    update();
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

void CSVRender::UnpagedWorldspaceWidget::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

    if (mime->fromDocument (mDocument))
    {
        const std::vector<CSMWorld::UniversalId>& data (mime->getData());
        CSVRender::WorldspaceWidget::dropType whatHappend = getDropType (data);

        switch (whatHappend)
        {
            case CSVRender::WorldspaceWidget::cellsExterior:
                emit exteriorCellsDropped(data);
                break;

            case CSVRender::WorldspaceWidget::cellsInterior:
                handleDrop(data);
                break;

            default:
                //not interior or exterior = either mixed or not actually cells. We don't need to do anything in this case.
                break;
        }
    } //not handling drops from different documents at the moment
}

void CSVRender::UnpagedWorldspaceWidget::handleDrop (const std::vector< CSMWorld::UniversalId >& data)
{
    mCellId = data.begin()->getId();
    update();
}
