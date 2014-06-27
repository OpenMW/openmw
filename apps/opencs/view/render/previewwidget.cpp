
#include "previewwidget.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"

CSVRender::PreviewWidget::PreviewWidget (CSMWorld::Data& data,
    const std::string& id, bool referenceable, QWidget *parent)
: SceneWidget (parent), mData (data),
  mObject (data, getSceneManager()->getRootSceneNode(), id, referenceable, true)
{
    setNavigation (&mOrbit);

    QAbstractItemModel *referenceables =
        mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables);

    connect (referenceables, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (ReferenceableDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (referenceables, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (ReferenceableAboutToBeRemoved (const QModelIndex&, int, int)));

    if (!referenceable)
    {
        QAbstractItemModel *references =
            mData.getTableModel (CSMWorld::UniversalId::Type_References);

        connect (references, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
            this, SLOT (ReferenceDataChanged (const QModelIndex&, const QModelIndex&)));
        connect (references, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
            this, SLOT (ReferenceAboutToBeRemoved (const QModelIndex&, int, int)));
    }
}

void CSVRender::PreviewWidget::ReferenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mObject.ReferenceableDataChanged (topLeft, bottomRight))
        flagAsModified();
}

void CSVRender::PreviewWidget::ReferenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (mObject.ReferenceableAboutToBeRemoved (parent, start, end))
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

void CSVRender::PreviewWidget::ReferenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mObject.ReferenceDataChanged (topLeft, bottomRight))
        flagAsModified();

    if (mObject.getReferenceId().empty())
        return;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int columnIndex = references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

    QModelIndex index = references.getModelIndex (mObject.getReferenceId(), columnIndex);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row())
        if (index.column()>=topLeft.column() && index.column()<=bottomRight.row())
            emit referenceableIdChanged (mObject.getReferenceableId());
}

void CSVRender::PreviewWidget::ReferenceAboutToBeRemoved (const QModelIndex& parent, int start,
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
