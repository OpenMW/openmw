#include "scriptedit.hpp"

#include <vector>

#include <QDragEnterEvent>

#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"

CSVWorld::ScriptEdit::ScriptEdit (QWidget* parent) :
QTextEdit(parent)
{

}

void CSVWorld::ScriptEdit::dragEnterEvent (QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVWorld::ScriptEdit::dragMoveEvent (QDragMoveEvent* event)
{
    event->accept();
}

void CSVWorld::ScriptEdit::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

    std::vector<CSMWorld::UniversalId> records (mime->getData());

    for (std::vector<CSMWorld::UniversalId>::iterator it = records.begin(); it != records.end(); ++it)
    {
        insertPlainText (QString::fromStdString (it->getId()));
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
