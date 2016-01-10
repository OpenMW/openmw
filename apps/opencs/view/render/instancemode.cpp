
#include "instancemode.hpp"

#include <QDragEnterEvent>

#include "../../model/prefs/state.hpp"

#include "../../model/world/idtable.hpp"
#include "../../model/world/commands.hpp"

#include "elements.hpp"
#include "object.hpp"
#include "worldspacewidget.hpp"

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":placeholder"), Element_Reference, "Instance editing",
  parent)
{

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
    getWorldspaceWidget().clearSelection (Element_Reference);

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

        /// \todo document check
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

        bool dropped = false;

        std::vector<CSMWorld::UniversalId> ids = mime->getData();

        CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_References));

        CSMWorld::IdTable& cellTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

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

                // increase reference count in cell
                QModelIndex countIndex = cellTable.getModelIndex (cellId,
                    cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_RefNumCounter));

                int count = cellTable.data (countIndex).toInt();

                std::auto_ptr<CSMWorld::ModifyCommand> incrementCommand (
                    new CSMWorld::ModifyCommand (cellTable, countIndex, count+1));

                document.getUndoStack().beginMacro (createCommand->text());
                document.getUndoStack().push (createCommand.release());
                document.getUndoStack().push (incrementCommand.release());
                document.getUndoStack().endMacro();

                dropped = true;
            }

        if (dropped)
            event->accept();
    }
}
