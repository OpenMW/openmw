#include "dragdroputils.hpp"

#include <QDropEvent>

#include "../../model/world/tablemimedata.hpp"

const CSMWorld::TableMimeData *CSVWorld::DragDropUtils::getTableMimeData(const QDropEvent &event)
{
    return dynamic_cast<const CSMWorld::TableMimeData *>(event.mimeData());
}

bool CSVWorld::DragDropUtils::canAcceptData(const QDropEvent &event, CSMWorld::ColumnBase::Display type)
{
    const CSMWorld::TableMimeData *data = getTableMimeData(event);
    return data != NULL && data->holdsType(type);
}

CSMWorld::UniversalId CSVWorld::DragDropUtils::getAcceptedData(const QDropEvent &event, 
                                                               CSMWorld::ColumnBase::Display type)
{
    if (canAcceptData(event, type))
    {
        return getTableMimeData(event)->returnMatching(type);
    }
    return CSMWorld::UniversalId::Type_None;
}
