#include <QDrag>

#include "../../model/world/tablemimedata.hpp"
#include "dragrecordtable.hpp"

void CSVWorld::DragRecordTable::startDrag (const CSVWorld::DragRecordTable& table)
{
    CSMWorld::TableMimeData* mime = new CSMWorld::TableMimeData (table.getDragedRecords(), mDocument);

    if (mime)
    {
        QDrag* drag = new QDrag (this);
        drag->setMimeData (mime);
        drag->setPixmap (QString::fromUtf8 (mime->getIcon().c_str()));
        drag->exec (Qt::CopyAction);
    }
}

CSVWorld::DragRecordTable::DragRecordTable (CSMDoc::Document& document) :
mDocument(document)
{}
