#include "recordstatusdelegate.hpp"

#include <QPainter>
#include <QApplication>
#include <QUndoStack>

#include "../../model/world/columns.hpp"

CSVWorld::RecordStatusDelegate::RecordStatusDelegate(const ValueList& values,
                                                     const IconList & icons,
                                                     CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent)
    : DataDisplayDelegate (values, icons, dispatcher, document,
                           "Records", "status-format",
                           parent)
{}

CSVWorld::CommandDelegate *CSVWorld::RecordStatusDelegateFactory::makeDelegate (
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    return new RecordStatusDelegate (mValues, mIcons, dispatcher, document, parent);
}

CSVWorld::RecordStatusDelegateFactory::RecordStatusDelegateFactory()
{
    std::vector<std::string> enums =
        CSMWorld::Columns::getEnums (CSMWorld::Columns::ColumnId_Modification);

    static const char *sIcons[] =
    {
        ":./base.png", ":./modified.png", ":./added.png", ":./removed.png", ":./removed.png", 0
    };

    for (int i=0; sIcons[i]; ++i)
        add (i, enums.at (i).c_str(), sIcons[i]);
}
