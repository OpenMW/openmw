#include "recordstatusdelegate.hpp"

#include <QPainter>
#include <QApplication>
#include <QUndoStack>

#include "../../model/settings/usersettings.hpp"
#include "../../model/world/columns.hpp"

CSVWorld::RecordStatusDelegate::RecordStatusDelegate(const ValueList& values,
                                                     const IconList & icons,
                                                     QUndoStack &undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack,
                           "Display Format", "Record Status Display",
                           parent)
{}

CSVWorld::CommandDelegate *CSVWorld::RecordStatusDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RecordStatusDelegate (mValues, mIcons, undoStack, parent);
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
