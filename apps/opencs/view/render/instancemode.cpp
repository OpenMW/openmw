
#include "instancemode.hpp"

#include <QDragEnterEvent>

#include "../../model/prefs/state.hpp"

#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/commands.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"

#include "mask.hpp"

#include "object.hpp"
#include "worldspacewidget.hpp"
#include "pagedworldspacewidget.hpp"
#include "instanceselectionmode.hpp"

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":placeholder"), Mask_Reference, "Instance editing",
  parent), mSubMode (0), mSelectionMode (0)
{
}

void CSVRender::InstanceMode::activate (CSVWidget::SceneToolbar *toolbar)
{
    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode (toolbar, "Edit Sub-Mode");
        mSubMode->addButton (":placeholder", "move",
            "Move selected instances"
            "<ul><li>Use primary edit to move instances around freely</li>"
            "<li>Use secondary edit to move instances around within the grid</li>"
            "</ul>"
            "<font color=Red>Not implemented yet</font color>");
        mSubMode->addButton (":placeholder", "rotate",
            "Rotate selected instances"
            "<ul><li>Use primary edit to rotate instances freely</li>"
            "<li>Use secondary edit to rotate instances within the grid</li>"
            "</ul>"
            "<font color=Red>Not implemented yet</font color>");
        mSubMode->addButton (":placeholder", "scale",
            "Scale selected instances"
            "<ul><li>Use primary edit to scale instances freely</li>"
            "<li>Use secondary edit to scale instances along the grid</li>"
            "</ul>"
            "<font color=Red>Not implemented yet</font color>");
    }

    if (!mSelectionMode)
        mSelectionMode = new InstanceSelectionMode (toolbar, getWorldspaceWidget());

    EditMode::activate (toolbar);

    toolbar->addTool (mSubMode);
    toolbar->addTool (mSelectionMode);
}

void CSVRender::InstanceMode::deactivate (CSVWidget::SceneToolbar *toolbar)
{
    if (mSelectionMode)
    {
        toolbar->removeTool (mSelectionMode);
        delete mSelectionMode;
        mSelectionMode = 0;
    }

    if (mSubMode)
    {
        toolbar->removeTool (mSubMode);
        delete mSubMode;
        mSubMode = 0;
    }

    EditMode::deactivate (toolbar);
}

void CSVRender::InstanceMode::primaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        primarySelectPressed (tag);
}

void CSVRender::InstanceMode::secondaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        secondarySelectPressed (tag);
}

void CSVRender::InstanceMode::primarySelectPressed (osg::ref_ptr<TagBase> tag)
{
    getWorldspaceWidget().clearSelection (Mask_Reference);

    if (tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            // hit an Object, select it
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (true);
            return;
        }
    }
}

void CSVRender::InstanceMode::secondarySelectPressed (osg::ref_ptr<TagBase> tag)
{
    if (tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            // hit an Object, toggle its selection state
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (!object->getSelected());
            return;
        }
    }
}

void CSVRender::InstanceMode::dragEnterEvent (QDragEnterEvent *event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData()))
    {
        if (!mime->fromDocument (getWorldspaceWidget().getDocument()))
            return;

        if (mime->holdsType (CSMWorld::UniversalId::Type_Referenceable))
            event->accept();
    }
}

void CSVRender::InstanceMode::dropEvent (QDropEvent* event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData()))
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();

        if (!mime->fromDocument (document))
            return;

        osg::Vec3f insertPoint = getWorldspaceWidget().getIntersectionPoint (event->pos());

        std::string cellId = getWorldspaceWidget().getCellId (insertPoint);

        CSMWorld::IdTree& cellTable = dynamic_cast<CSMWorld::IdTree&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

        bool noCell = document.getData().getCells().searchId (cellId)==-1;

        if (noCell)
        {
            std::string mode = CSMPrefs::get()["Scene Drops"]["outside-drop"].toString();

            // target cell does not exist
            if (mode=="Discard")
                return;

            if (mode=="Create cell and insert")
            {
                std::auto_ptr<CSMWorld::CreateCommand> createCommand (
                    new CSMWorld::CreateCommand (cellTable, cellId));

                int parentIndex = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
                int index = cellTable.findNestedColumnIndex (parentIndex, CSMWorld::Columns::ColumnId_Interior);
                createCommand->addNestedValue (parentIndex, index, false);

                document.getUndoStack().push (createCommand.release());

                if (CSVRender::PagedWorldspaceWidget *paged =
                    dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
                {
                    CSMWorld::CellSelection selection = paged->getCellSelection();
                    selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                    paged->setCellSelection (selection);
                }

                noCell = false;
            }
        }
        else if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            CSMWorld::CellSelection selection = paged->getCellSelection();
            if (!selection.has (CSMWorld::CellCoordinates::fromId (cellId).first))
            {
                // target cell exists, but is not shown
                std::string mode =
                    CSMPrefs::get()["Scene Drops"]["outside-visible-drop"].toString();

                if (mode=="Discard")
                    return;

                if (mode=="Show cell and insert")
                {
                    selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                    paged->setCellSelection (selection);
                }
            }
        }

        CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_References));

        bool dropped = false;

        std::vector<CSMWorld::UniversalId> ids = mime->getData();

        for (std::vector<CSMWorld::UniversalId>::const_iterator iter (ids.begin());
            iter!=ids.end(); ++iter)
            if (mime->isReferencable (iter->getType()))
            {
                // create reference
                std::auto_ptr<CSMWorld::CreateCommand> createCommand (
                    new CSMWorld::CreateCommand (
                    referencesTable, document.getData().getReferences().getNewId()));

                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_Cell), QString::fromUtf8 (cellId.c_str()));
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionXPos), insertPoint.x());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionYPos), insertPoint.y());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionZPos), insertPoint.z());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_ReferenceableId),
                     QString::fromUtf8 (iter->getId().c_str()));

                std::auto_ptr<CSMWorld::ModifyCommand> incrementCommand;

                if (!noCell)
                {
                    // increase reference count in cell
                    QModelIndex countIndex = cellTable.getModelIndex (cellId,
                        cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNumCounter));

                    int count = cellTable.data (countIndex).toInt();

                    incrementCommand.reset (
                        new CSMWorld::ModifyCommand (cellTable, countIndex, count+1));
                }

                document.getUndoStack().beginMacro (createCommand->text());
                document.getUndoStack().push (createCommand.release());
                if (incrementCommand.get())
                    document.getUndoStack().push (incrementCommand.release());
                document.getUndoStack().endMacro();

                dropped = true;
            }

        if (dropped)
            event->accept();
    }
}
