
#include "instanceselectionmode.hpp"

#include <QMenu>
#include <QAction>

#include "worldspacewidget.hpp"

bool CSVRender::InstanceSelectionMode::createContextMenu (QMenu *menu)
{
    if (menu)
    {
        menu->addAction (mSelectAll);
        menu->addAction (mDeselectAll);
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

    mSelectAll = new QAction ("Select all Instances", this);
    mDeselectAll = new QAction ("Clear selection", this);

    connect (mSelectAll, SIGNAL (triggered ()), this, SLOT (selectAll()));
    connect (mDeselectAll, SIGNAL (triggered ()), this, SLOT (clearSelection()));
}

void CSVRender::InstanceSelectionMode::selectAll()
{
    mWorldspaceWidget.selectAll (Mask_Reference);
}

void CSVRender::InstanceSelectionMode::clearSelection()
{
    mWorldspaceWidget.clearSelection (Mask_Reference);
}
