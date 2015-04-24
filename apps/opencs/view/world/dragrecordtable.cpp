#include <QDrag>

#include "../../model/world/tablemimedata.hpp"
#include "dragrecordtable.hpp"

void CSVWorld::DragRecordTable::startDragFromTable (const CSVWorld::DragRecordTable& table)
{
    CSMWorld::TableMimeData* mime = new CSMWorld::TableMimeData (table.getDraggedRecords(), mDocument);

    if (mime)
    {
        QDrag* drag = new QDrag (this);
        drag->setMimeData (mime);
        drag->setPixmap (QString::fromUtf8 (mime->getIcon().c_str()));
        drag->exec (Qt::CopyAction);
    }
}

CSVWorld::DragRecordTable::DragRecordTable (CSMDoc::Document& document, QWidget* parent) :
mDocument(document),
QTableView(parent),
mEditLock(false)
{}

void CSVWorld::DragRecordTable::setEditLock (bool locked)
{
    mEditLock = locked;
}

void CSVWorld::DragRecordTable::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void CSVWorld::DragRecordTable::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}
