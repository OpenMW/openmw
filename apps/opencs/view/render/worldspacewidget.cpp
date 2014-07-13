
#include "worldspacewidget.hpp"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <QtGui/qevent.h>

#include "../../model/world/universalid.hpp"

#include "../widget/scenetoolmode.hpp"

CSVRender::WorldspaceWidget::WorldspaceWidget (CSMDoc::Document& document, QWidget* parent)
: SceneWidget (parent), mDocument(document)
{
    setAcceptDrops(true);

    QAbstractItemModel *referenceables =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables);

    connect (referenceables, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (referenceableDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (referenceables, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (referenceableAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (referenceables, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (referenceableAdded (const QModelIndex&, int, int)));

    QAbstractItemModel *references =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_References);

    connect (references, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (referenceDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (references, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (referenceAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (references, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (referenceAdded (const QModelIndex&, int, int)));
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

CSVWidget::SceneToolMode *CSVRender::WorldspaceWidget::makeNavigationSelector (
    CSVWidget::SceneToolbar *parent)
{
    CSVWidget::SceneToolMode *tool = new CSVWidget::SceneToolMode (parent);

    /// \todo replace icons
    /// \todo consider user-defined button-mapping
    tool->addButton (":door.png", "1st",
        "First Person"
        "<ul><li>Mouse-Look while holding the left button</li>"
        "<li>WASD movement keys</li>"
        "<li>Mouse wheel moves the camera forawrd/backward</li>"
        "<li>Stafing (also vertically) by holding the left mouse button and control</li>"
        "<li>Camera is held upright</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");
    tool->addButton (":GMST.png", "free",
        "Free Camera"
        "<ul><li>Mouse-Look while holding the left button</li>"
        "<li>Stafing (also vertically) via WASD or by holding the left mouse button and control</li>"
        "<li>Mouse wheel moves the camera forawrd/backward</li>"
        "<li>Roll camera with Q and E keys</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");
    tool->addButton (":Info.png", "orbit",
        "Orbiting Camera"
        "<ul><li>Always facing the centre point</li>"
        "<li>Rotate around the centre point via WASD or by moving the mouse while holding the left button</li>"
        "<li>Mouse wheel moves camera away or towards centre point but can not pass through it</li>"
        "<li>Roll camera with Q and E keys</li>"
        "<li>Stafing (also vertically) by holding the left mouse button and control (includes relocation of the centre point)</li>"
        "<li>Hold shift to speed up movement</li>"
        "</ul>");

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
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    if (mime->fromDocument (mDocument))
    {
        emit dataDropped(mime->getData());
    } //not handling drops from different documents at the moment
}
