
#include "previewwidget.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"

void CSVRender::PreviewWidget::setup()
{
    setNavigation (&mOrbit);

    mNode = getSceneManager()->getRootSceneNode()->createChildSceneNode();
    mNode->setPosition (Ogre::Vector3 (0, 0, 0));

    setModel();

    QAbstractItemModel *referenceables =
        mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables);

    connect (referenceables, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (ReferenceableDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (referenceables, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (ReferenceableAboutToBeRemoved (const QModelIndex&, int, int)));
}

void CSVRender::PreviewWidget::setModel()
{
    if (mNode)
    {
        mObject.setNull();

        if (mReferenceableId.empty())
            return;

        int column =
            mData.getReferenceables().findColumnIndex (CSMWorld::Columns::ColumnId_Model);

        int row = mData.getReferenceables().searchId (mReferenceableId);

        if (row==-1)
            return;

        QVariant value = mData.getReferenceables().getData (row, column);

        if (!value.isValid())
            return;

        std::string model = value.toString().toUtf8().constData();

        if (model.empty())
            return;

        mObject = NifOgre::Loader::createObjects (mNode, "Meshes\\" + model);
    }
}

void CSVRender::PreviewWidget::adjust()
{
    if (mNode)
    {
        int row = mData.getReferences().getIndex (mReferenceId);

        float scale = mData.getReferences().getData (row, mData.getReferences().
            findColumnIndex (CSMWorld::Columns::ColumnId_Scale)).toFloat();
        float rotX = mData.getReferences().getData (row, mData.getReferences().
            findColumnIndex (CSMWorld::Columns::ColumnId_PositionXRot)).toFloat();
        float rotY = mData.getReferences().getData (row, mData.getReferences().
            findColumnIndex (CSMWorld::Columns::ColumnId_PositionYRot)).toFloat();
        float rotZ = mData.getReferences().getData (row, mData.getReferences().
            findColumnIndex (CSMWorld::Columns::ColumnId_PositionZRot)).toFloat();

        mNode->setScale (scale, scale, scale);

        Ogre::Quaternion xr (Ogre::Radian(-rotX), Ogre::Vector3::UNIT_X);

        Ogre::Quaternion yr (Ogre::Radian(-rotY), Ogre::Vector3::UNIT_Y);

        Ogre::Quaternion zr (Ogre::Radian(-rotZ), Ogre::Vector3::UNIT_Z);

        mNode->setOrientation (xr*yr*zr);
    }
}

CSVRender::PreviewWidget::PreviewWidget (CSMWorld::Data& data,
    const std::string& referenceableId, QWidget *parent)
: SceneWidget (parent), mData (data), mNode (0), mReferenceableId (referenceableId)
{
    setup();
}

CSVRender::PreviewWidget::PreviewWidget (CSMWorld::Data& data,
    const std::string& referenceableId, const std::string& referenceId, QWidget *parent)
: SceneWidget (parent), mData (data), mReferenceableId (referenceableId),
  mReferenceId (referenceId)
{
    setup();

    adjust();

    QAbstractItemModel *references =
        mData.getTableModel (CSMWorld::UniversalId::Type_References);

    connect (references, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (ReferenceDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (references, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (ReferenceAboutToBeRemoved (const QModelIndex&, int, int)));
}

void CSVRender::PreviewWidget::ReferenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mReferenceableId.empty())
        return;

    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    QModelIndex index = referenceables.getModelIndex (mReferenceableId, 0);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row())
    {
        /// \todo possible optimisation; check columns and only update if relevant columns have
        /// changed
        setModel();
        flagAsModified();
    }
}

void CSVRender::PreviewWidget::ReferenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (mReferenceableId.empty())
        return;

    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    QModelIndex index = referenceables.getModelIndex (mReferenceableId, 0);

    if (index.row()>=start && index.row()<=end)
    {
        if (mReferenceId.empty())
        {
            // this is a preview for a referenceble
            emit closeRequest();
        }
        else
        {
            // this is a preview for a reference
            mObject.setNull();
            flagAsModified();
        }
    }
}

void CSVRender::PreviewWidget::ReferenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mReferenceId.empty())
        return;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int columnIndex = references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

    QModelIndex index = references.getModelIndex (mReferenceId, columnIndex);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row())
    {
        /// \todo possible optimisation; check columns and only update if relevant columns have
        /// changed
        adjust();

        if (index.column()>=topLeft.column() && index.column()<=bottomRight.row())
        {
            mReferenceableId = references.data (index).toString().toUtf8().constData();
            emit referenceableIdChanged (mReferenceableId);
            setModel();
        }

        flagAsModified();
    }
}

void CSVRender::PreviewWidget::ReferenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (mReferenceId.empty())
        return;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    QModelIndex index = references.getModelIndex (mReferenceId, 0);

    if (index.row()>=start && index.row()<=end)
        emit closeRequest();
}
