
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
#include "instancemovemode.hpp"

int CSVRender::InstanceMode::getSubModeFromId (const std::string& id) const
{
    return id=="move" ? 0 : (id=="rotate" ? 1 : 2);
}

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":placeholder"), Mask_Reference, "Instance editing",
  parent), mSubMode (0), mSubModeId ("move"), mSelectionMode (0), mDragMode (DragMode_None),
  mDragAxis (-1), mLocked (false)
{
}

void CSVRender::InstanceMode::activate (CSVWidget::SceneToolbar *toolbar)
{
    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode (toolbar, "Edit Sub-Mode");
        mSubMode->addButton (new InstanceMoveMode (this), "move");
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

        mSubMode->setButton (mSubModeId);

        connect (mSubMode, SIGNAL (modeChanged (const std::string&)),
            this, SLOT (subModeChanged (const std::string&)));
    }

    if (!mSelectionMode)
        mSelectionMode = new InstanceSelectionMode (toolbar, getWorldspaceWidget());

    mDragMode = DragMode_None;

    EditMode::activate (toolbar);

    toolbar->addTool (mSubMode);
    toolbar->addTool (mSelectionMode);

    std::string subMode = mSubMode->getCurrentId();

    getWorldspaceWidget().setSubMode (getSubModeFromId (subMode), Mask_Reference);
}

void CSVRender::InstanceMode::deactivate (CSVWidget::SceneToolbar *toolbar)
{
    mDragMode = DragMode_None;
    getWorldspaceWidget().reset (Mask_Reference);

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

void CSVRender::InstanceMode::setEditLock (bool locked)
{
    mLocked = locked;

    if (mLocked)
        getWorldspaceWidget().abortDrag();
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

bool CSVRender::InstanceMode::primaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    if (mDragMode!=DragMode_None || mLocked)
        return false;

    if (tag && CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
    {
        getWorldspaceWidget().clearSelection (Mask_Reference);
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (true);
        }
    }

    std::vector<osg::ref_ptr<TagBase> > selection =
        getWorldspaceWidget().getSelection (Mask_Reference);

    if (selection.empty())
        return false;

    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
        iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            objectTag->mObject->setEdited (Object::Override_Position);
        }
    }

    // \todo check for sub-mode

    if (CSVRender::ObjectMarkerTag *objectTag = dynamic_cast<CSVRender::ObjectMarkerTag *> (tag.get()))
    {
        mDragAxis = objectTag->mAxis;
    }
    else
        mDragAxis = -1;

    mDragMode = DragMode_Move;

    return true;
}

bool CSVRender::InstanceMode::secondaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    if (mLocked)
        return false;

    return false;
}

void CSVRender::InstanceMode::drag (int diffX, int diffY, double speedFactor)
{
    osg::Vec3f eye;
    osg::Vec3f centre;
    osg::Vec3f up;

    getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, centre, up);

    osg::Vec3f offset;

    if (diffY)
        offset += up * diffY * speedFactor;

    if (diffX)
        offset += ((centre-eye) ^ up) * diffX * speedFactor;

    switch (mDragMode)
    {
        case DragMode_Move:
        {
            if (mDragAxis!=-1)
                for (int i=0; i<3; ++i)
                    if (i!=mDragAxis)
                        offset[i] = 0;

            std::vector<osg::ref_ptr<TagBase> > selection =
                getWorldspaceWidget().getEdited (Mask_Reference);

            for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
                iter!=selection.end(); ++iter)
            {
                if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
                {
                    ESM::Position position = objectTag->mObject->getPosition();
                    for (int i=0; i<3; ++i)
                        position.pos[i] += offset[i];
                    objectTag->mObject->setPosition (position.pos);
                }
            }

            break;
        }

        case DragMode_None: break;
    }
}

void CSVRender::InstanceMode::dragCompleted()
{
    std::vector<osg::ref_ptr<TagBase> > selection =
        getWorldspaceWidget().getEdited (Mask_Reference);

    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();

    QString description;

    switch (mDragMode)
    {
        case DragMode_Move: description = "Move Instances"; break;

        case DragMode_None: break;
    }

    undoStack.beginMacro (description);

    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
        iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            objectTag->mObject->apply (undoStack);
        }
    }

    undoStack.endMacro();

    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragAborted()
{
    getWorldspaceWidget().reset (Mask_Reference);
    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragWheel (int diff, double speedFactor)
{
    if (mDragMode==DragMode_Move)
    {
        osg::Vec3f eye;
        osg::Vec3f centre;
        osg::Vec3f up;

        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, centre, up);

        osg::Vec3f offset = centre - eye;
        offset.normalize();
        offset *= diff * speedFactor;

        std::vector<osg::ref_ptr<TagBase> > selection =
            getWorldspaceWidget().getEdited (Mask_Reference);

        for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
            iter!=selection.end(); ++iter)
        {
            if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
            {
                ESM::Position position = objectTag->mObject->getPosition();
                for (int i=0; i<3; ++i)
                    position.pos[i] += offset[i];
                objectTag->mObject->setPosition (position.pos);
            }
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

int CSVRender::InstanceMode::getSubMode() const
{
    return mSubMode ? getSubModeFromId (mSubMode->getCurrentId()) : 0;
}

void CSVRender::InstanceMode::subModeChanged (const std::string& id)
{
    mSubModeId = id;
    getWorldspaceWidget().abortDrag();
    getWorldspaceWidget().setSubMode (getSubModeFromId (id), Mask_Reference);
}
