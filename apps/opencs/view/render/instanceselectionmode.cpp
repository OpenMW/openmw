
#include "instanceselectionmode.hpp"

#include <QMenu>
#include <QAction>

#include "../../model/world/idtable.hpp"
#include "../../model/world/commands.hpp"

#include "worldspacewidget.hpp"
#include "object.hpp"

bool CSVRender::InstanceSelectionMode::createContextMenu (QMenu *menu)
{
    if (menu)
    {
        menu->addAction (mSelectAll);
        menu->addAction (mDeselectAll);
        menu->addAction (mSelectSame);
        menu->addAction (mDeleteSelection);
    }

    return true;
}

CSVRender::InstanceSelectionMode::InstanceSelectionMode (CSVWidget::SceneToolbar *parent,
    WorldspaceWidget& worldspaceWidget)
: CSVWidget::SceneToolMode (parent, "Selection Mode"), mWorldspaceWidget (worldspaceWidget)
{
    addButton (":placeholder", "cube-centre",
        "Centred cube"
        "<ul><li>Drag with primary (make instances the selection) or secondary (invert selection state) select button from the centre of the selection cube outwards</li>"
        "<li>The selection cube is aligned to the word space axis</li>"
        "<li>If context selection mode is enabled, a drag with primary/secondary edit not starting on an instance will have the same effect</li>"
        "</ul>"
        "<font color=Red>Not implemented yet</font color>");
    addButton (":placeholder", "cube-corner",
        "Cube corner to corner"
        "<ul><li>Drag with primary (make instances the selection) or secondary (invert selection state) select button from one corner of the selection cube to the opposite corner</li>"
        "<li>The selection cube is aligned to the word space axis</li>"
        "<li>If context selection mode is enabled, a drag with primary/secondary edit not starting on an instance will have the same effect</li>"
        "</ul>"
        "<font color=Red>Not implemented yet</font color>");
    addButton (":placeholder", "sphere",
        "Centred sphere"
        "<ul><li>Drag with primary (make instances the selection) or secondary (invert selection state) select button from the centre of the selection sphere outwards</li>"
        "<li>If context selection mode is enabled, a drag with primary/secondary edit not starting on an instance will have the same effect</li>"
        "</ul>"
        "<font color=Red>Not implemented yet</font color>");

    mSelectAll = new QAction ("Select all instances", this);
    mDeselectAll = new QAction ("Clear selection", this);
    mDeleteSelection = new QAction ("Delete selected instances", this);
    mSelectSame = new QAction ("Extend selection to instances with same object ID", this);
    connect (mSelectAll, SIGNAL (triggered ()), this, SLOT (selectAll()));
    connect (mDeselectAll, SIGNAL (triggered ()), this, SLOT (clearSelection()));
    connect (mDeleteSelection, SIGNAL (triggered ()), this, SLOT (deleteSelection()));
    connect (mSelectSame, SIGNAL (triggered ()), this, SLOT (selectSame()));
}

void CSVRender::InstanceSelectionMode::selectAll()
{
    mWorldspaceWidget.selectAll (Mask_Reference);
}

void CSVRender::InstanceSelectionMode::clearSelection()
{
    mWorldspaceWidget.clearSelection (Mask_Reference);
}

void CSVRender::InstanceSelectionMode::deleteSelection()
{
    std::vector<osg::ref_ptr<TagBase> > selection =
        mWorldspaceWidget.getSelection (Mask_Reference);

    CSMWorld::IdTable& referencesTable =
            dynamic_cast<CSMWorld::IdTable&> (*mWorldspaceWidget.getDocument().getData().
            getTableModel (CSMWorld::UniversalId::Type_References));

    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
        iter!=selection.end(); ++iter)
    {
        CSMWorld::DeleteCommand *command = new CSMWorld::DeleteCommand (referencesTable,
            static_cast<ObjectTag *> (iter->get())->mObject->getReferenceId());

        mWorldspaceWidget.getDocument().getUndoStack().push (command);
    }
}

void CSVRender::InstanceSelectionMode::selectSame()
{
    mWorldspaceWidget.selectAllWithSameParentId (Mask_Reference);
}
