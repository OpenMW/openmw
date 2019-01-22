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
    std::vector<std::pair<int,std::string>> enums =
        CSMWorld::Columns::getEnums (CSMWorld::Columns::ColumnId_Modification);

    static const char *sIcons[] =
    {
        ":list-base", ":list-modified", ":list-added", ":list-removed", ":list-removed", 0
    };

    for (int i=0; sIcons[i]; ++i)
    {
        auto& enumPair = enums.at(i);
        add (enumPair.first, enumPair.second.c_str(), sIcons[i]);
    }
}
