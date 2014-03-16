
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

    Ogre::SceneNode* node = getSceneManager()->getRootSceneNode()->createChildSceneNode();
    node->setPosition (Ogre::Vector3 (0, 0, 0));

    mObject = NifOgre::Loader::createObjects (node, "Meshes\\" + model);
}

CSVRender::PreviewWidget::PreviewWidget (const CSMWorld::Data& data,
    const std::string& referenceableId, QWidget *parent)
: SceneWidget (parent), mData (data)
{
    setup (referenceableId);
}

CSVRender::PreviewWidget::PreviewWidget (const CSMWorld::Data& data,
    const std::string& referenceableId, const std::string& referenceId, QWidget *parent)
: SceneWidget (parent), mData (data)
{
    setup (referenceableId);
    /// \todo apply reference modifications (scale, rotation)
}
