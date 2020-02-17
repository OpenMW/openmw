#include "instanceselectionmode.hpp"

#include <QMenu>
#include <QAction>

#include "../../model/world/idtable.hpp"
#include "../../model/world/commands.hpp"

#include <components/sceneutil/vismask.hpp>

#include "worldspacewidget.hpp"
#include "object.hpp"

namespace CSVRender
{
    InstanceSelectionMode::InstanceSelectionMode(CSVWidget::SceneToolbar* parent, WorldspaceWidget& worldspaceWidget)
        : SelectionMode(parent, worldspaceWidget, SceneUtil::Mask_EditorReference)
    {
        mSelectSame = new QAction("Extend selection to instances with same object ID", this);
        mDeleteSelection = new QAction("Delete selected instances", this);

        connect(mSelectSame, SIGNAL(triggered()), this, SLOT(selectSame()));
        connect(mDeleteSelection, SIGNAL(triggered()), this, SLOT(deleteSelection()));
    }

    bool InstanceSelectionMode::createContextMenu(QMenu* menu)
    {
        if (menu)
        {
            SelectionMode::createContextMenu(menu);

            menu->addAction(mSelectSame);
            menu->addAction(mDeleteSelection);
        }

        return true;
    }

    void InstanceSelectionMode::selectSame()
    {
        getWorldspaceWidget().selectAllWithSameParentId(SceneUtil::Mask_EditorReference);
    }

    void InstanceSelectionMode::deleteSelection()
    {
        std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection(SceneUtil::Mask_EditorReference);

        CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&>(
            *getWorldspaceWidget().getDocument().getData().getTableModel(CSMWorld::UniversalId::Type_References));

        for (std::vector<osg::ref_ptr<TagBase> >::iterator iter = selection.begin(); iter != selection.end(); ++iter)
        {
            CSMWorld::DeleteCommand* command = new CSMWorld::DeleteCommand(referencesTable,
                static_cast<ObjectTag*>(iter->get())->mObject->getReferenceId());

            getWorldspaceWidget().getDocument().getUndoStack().push(command);
        }
    }
}
