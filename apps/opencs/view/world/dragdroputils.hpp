#ifndef CSV_WORLD_DRAGDROPUTILS_HPP
#define CSV_WORLD_DRAGDROPUTILS_HPP

#include "../../model/world/columnbase.hpp"

class QDropEvent;

namespace CSMWorld
{
    class TableMimeData;
    class UniversalId;
}

namespace CSVWorld
{
    namespace DragDropUtils
    {
        const CSMWorld::TableMimeData *getTableMimeData(const QDropEvent &event);

        bool canAcceptData(const QDropEvent &event, CSMWorld::ColumnBase::Display type);
        ///< Checks whether the \a event contains a valid CSMWorld::TableMimeData that holds the \a type

        bool isInfo(const QDropEvent &event, CSMWorld::ColumnBase::Display type);
        ///< Info types can be dragged to sort the info table

        CSMWorld::UniversalId getAcceptedData(const QDropEvent &event, CSMWorld::ColumnBase::Display type);
        ///< Gets the accepted data from the \a event using the \a type
        ///< \return Type_None if the \a event data doesn't holds the \a type
    }
}

#endif
