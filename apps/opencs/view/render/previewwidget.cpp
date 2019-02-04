#include "previewwidget.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"

CSVRender::PreviewWidget::PreviewWidget (CSMWorld::Data& data,
    const std::string& id, bool referenceable, QWidget *parent)
: SceneWidget (data.getResourceSystem(), parent), mData (data), mObject(data, mRootNode, id, referenceable)
{
    selectNavigationMode("orbit");

    QAbstractItemModel *referenceables =
        mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables);

    connect (referenceables, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (referenceableDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (referenceables, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (referenceableAboutToBeRemoved (const QModelIndex&, int, int)));

    connect (&mData, SIGNAL (assetTablesChanged ()),
        this, SLOT (assetTablesChanged ()));

    setExterior(false);

    if (!referenceable)
    {
        QAbstractItemModel *references =
            mData.getTableModel (CSMWorld::UniversalId::Type_References);

        connect (references, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (referenceDataChanged (const QModelIndex&, const QModelIndex&)));
        connect (references, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
            this, SLOT (referenceAboutToBeRemoved (const QModelIndex&, int, int)));
    }
}

void CSVRender::PreviewWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mObject.referenceableDataChanged (topLeft, bottomRight))
        flagAsModified();

    if (mObject.getReferenceId().empty())
    {
        CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
            *mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables));

        QModelIndex index = referenceables.getModelIndex (mObject.getReferenceableId(),
            referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Modification));

        if (referenceables.data (index).toInt()==CSMWorld::RecordBase::State_Deleted)
            emit closeRequest();
    }
}

void CSVRender::PreviewWidget::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (mObject.referenceableAboutToBeRemoved (parent, start, end))
        flagAsModified();

    if (mObject.getReferenceableId().empty())
        return;

    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    QModelIndex index = referenceables.getModelIndex (mObject.getReferenceableId(), 0);

    if (index.row()>=start && index.row()<=end)
    {
        if (mObject.getReferenceId().empty())
        {
            // this is a preview for a referenceble
            emit closeRequest();
        }
    }
}

void CSVRender::PreviewWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mObject.referenceDataChanged (topLeft, bottomRight))
        flagAsModified();

    if (mObject.getReferenceId().empty())
        return;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    // check for deleted state
    {
        QModelIndex index = references.getModelIndex (mObject.getReferenceId(),
            references.findColumnIndex (CSMWorld::Columns::ColumnId_Modification));

        if (references.data (index).toInt()==CSMWorld::RecordBase::State_Deleted)
        {
            emit closeRequest();
            return;
        }
    }

    int columnIndex = references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

    QModelIndex index = references.getModelIndex (mObject.getReferenceId(), columnIndex);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row())
        if (index.column()>=topLeft.column() && index.column()<=bottomRight.row())
            emit referenceableIdChanged (mObject.getReferenceableId());
}

void CSVRender::PreviewWidget::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (mObject.getReferenceId().empty())
        return;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    QModelIndex index = references.getModelIndex (mObject.getReferenceId(), 0);

    if (index.row()>=start && index.row()<=end)
        emit closeRequest();
}

void CSVRender::PreviewWidget::assetTablesChanged ()
{
    mObject.reloadAssets();
}
