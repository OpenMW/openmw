
#include "worldspacewidget.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <QtGui/qevent.h>

#include "../world/scenetoolmode.hpp"
#include <apps/opencs/model/world/universalid.hpp>

CSVRender::WorldspaceWidget::WorldspaceWidget (const CSMDoc::Document& document, QWidget* parent)
: SceneWidget (parent), mDocument(document)
{
    Ogre::Entity* ent = getSceneManager()->createEntity("cube", Ogre::SceneManager::PT_CUBE);
    ent->setMaterialName("BaseWhite");

    getSceneManager()->getRootSceneNode()->attachObject(ent);

    setAcceptDrops(true);
}

void CSVRender::WorldspaceWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
        setNavigation (&m1st);
    else if (mode=="free")
        setNavigation (&mFree);
    else if (mode=="orbit")
        setNavigation (&mOrbit);
}

void CSVRender::WorldspaceWidget::useViewHint (const std::string& hint) {}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    setNavigation (&m1st);
}

CSVWorld::SceneToolMode *CSVRender::WorldspaceWidget::makeNavigationSelector (
    CSVWorld::SceneToolbar *parent)
{
    CSVWorld::SceneToolMode *tool = new CSVWorld::SceneToolMode (parent);

    tool->addButton (":door.png", "1st"); /// \todo replace icons
    tool->addButton (":GMST.png", "free");
    tool->addButton (":Info.png", "orbit");

    connect (tool, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (selectNavigationMode (const std::string&)));

    return tool;
}

CSVRender::WorldspaceWidget::dropType CSVRender::WorldspaceWidget::getDropType (
    const std::vector< CSMWorld::UniversalId >& data)
{
    dropType output = notCells;
    bool firstIteration = true;

    for (unsigned i = 0; i < data.size(); ++i)
    {
        if (data[i].getType() == CSMWorld::UniversalId::Type_Cell ||
                data[i].getType() == CSMWorld::UniversalId::Type_Cell_Missing)
        {
            if (*(data[i].getId().begin()) == '#') //exterior
            {
                if (firstIteration)
                {
                    output = cellsExterior;
                    firstIteration = false;
                    continue;
                }

                if (output == cellsInterior)
                {
                    output = cellsMixed;
                    break;
                } else {
                    output = cellsInterior;
                }
            } else //interior
            {
                if (firstIteration)
                {
                    output = cellsInterior;
                    firstIteration = false;
                    continue;
                }

                if (output == cellsExterior)
                {
                    output = cellsMixed;
                    break;
                } else {
                    output = cellsInterior;
                }
            }
        } else {
            output = notCells;
            break;
        }
    }

    return output;
}

void CSVRender::WorldspaceWidget::dragEnterEvent (QDragEnterEvent* event)
{
    event->accept();
}

void CSVRender::WorldspaceWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}


void CSVRender::WorldspaceWidget::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

    if (mime->fromDocument (mDocument))
    {
        emit dataDropped(mime->getData());
    } //not handling drops from different documents at the moment
}