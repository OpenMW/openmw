
#include "previewwidget.hpp"

#include <OgreSceneManager.h>

#include "../../model/world/data.hpp"

void CSVRender::PreviewWidget::setup (const std::string& id)
{
    setNavigation (&mOrbit);

    int column = mData.getReferenceables().findColumnIndex (CSMWorld::Columns::ColumnId_Model);

    int row = mData.getReferenceables().getIndex (id);

    QVariant value = mData.getReferenceables().getData (row, column);

    if (!value.isValid())
        return;

    std::string model = value.toString().toUtf8().constData();

    if (model.empty())
        return;

    mNode = getSceneManager()->getRootSceneNode()->createChildSceneNode();
    mNode->setPosition (Ogre::Vector3 (0, 0, 0));

    mObject = NifOgre::Loader::createObjects (mNode, "Meshes\\" + model);
}

void CSVRender::PreviewWidget::adjust (const std::string& id)
{
    if (mNode)
    {
        int row = mData.getReferences().getIndex (id);

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

CSVRender::PreviewWidget::PreviewWidget (const CSMWorld::Data& data,
    const std::string& referenceableId, QWidget *parent)
: SceneWidget (parent), mData (data), mNode (0)
{
    setup (referenceableId);
}

CSVRender::PreviewWidget::PreviewWidget (const CSMWorld::Data& data,
    const std::string& referenceableId, const std::string& referenceId, QWidget *parent)
: SceneWidget (parent), mData (data)
{
    setup (referenceableId);
    adjust (referenceId);
}
