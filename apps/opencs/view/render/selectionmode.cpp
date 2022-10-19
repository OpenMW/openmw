#include "selectionmode.hpp"

#include <QAction>
#include <QMenu>

#include <memory>

#include <apps/opencs/view/widget/scenetoolmode.hpp>

#include "worldspacewidget.hpp"

namespace CSVWidget
{
    class SceneToolbar;
}

namespace CSVRender
{
    SelectionMode::SelectionMode(
        CSVWidget::SceneToolbar* parent, WorldspaceWidget& worldspaceWidget, unsigned int interactionMask)
        : SceneToolMode(parent, "Selection mode")
        , mWorldspaceWidget(worldspaceWidget)
        , mInteractionMask(interactionMask)
    {
        addButton(":scenetoolbar/selection-mode-cube", "cube-centre",
            "Centred cube"
            "<ul><li>Drag with {scene-select-primary} for primary select or {scene-select-secondary} for secondary "
            "select "
            "from the centre of the selection cube outwards.</li>"
            "<li>The selection cube is aligned to the word space axis</li>"
            "<li>If context selection mode is enabled, a drag with {scene-edit-primary} or {scene-edit-secondary} not "
            "starting on an instance will have the same effect</li>"
            "</ul>");
        addButton(":scenetoolbar/selection-mode-cube-corner", "cube-corner",
            "Cube corner to corner"
            "<ul><li>Drag with {scene-select-primary} for primary select or {scene-select-secondary} for secondary "
            "select "
            "from one corner of the selection cube to the opposite corner</li>"
            "<li>The selection cube is aligned to the word space axis</li>"
            "<li>If context selection mode is enabled, a drag with {scene-edit-primary} or {scene-edit-secondary} not "
            "starting on an instance will have the same effect</li>"
            "</ul>");
        addButton(":scenetoolbar/selection-mode-cube-sphere", "sphere",
            "Centred sphere"
            "<ul><li>Drag with {scene-select-primary} for primary select or {scene-select-secondary} for secondary "
            "select  "
            "from the centre of the selection sphere outwards</li>"
            "<li>If context selection mode is enabled, a drag with {scene-edit-primary} or {scene-edit-secondary} not "
            "starting on an instance will have the same effect</li>"
            "</ul>");

        mSelectAll = new QAction("Select all", this);
        mDeselectAll = new QAction("Clear selection", this);
        mInvertSelection = new QAction("Invert selection", this);

        connect(mSelectAll, &QAction::triggered, this, &SelectionMode::selectAll);
        connect(mDeselectAll, &QAction::triggered, this, &SelectionMode::clearSelection);
        connect(mInvertSelection, &QAction::triggered, this, &SelectionMode::invertSelection);
    }

    WorldspaceWidget& SelectionMode::getWorldspaceWidget()
    {
        return mWorldspaceWidget;
    }

    bool SelectionMode::createContextMenu(QMenu* menu)
    {
        if (menu)
        {
            menu->addAction(mSelectAll);
            menu->addAction(mDeselectAll);
            menu->addAction(mInvertSelection);
        }

        return true;
    }

    void SelectionMode::selectAll()
    {
        getWorldspaceWidget().selectAll(mInteractionMask);
    }

    void SelectionMode::clearSelection()
    {
        getWorldspaceWidget().clearSelection(mInteractionMask);
    }

    void SelectionMode::invertSelection()
    {
        getWorldspaceWidget().invertSelection(mInteractionMask);
    }
}
