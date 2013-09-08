#include "recordstatusdelegate.hpp"

#include <QPainter>
#include <QApplication>
#include <QUndoStack>

#include "../../model/settings/usersettings.hpp"
#include "../../model/world/columns.hpp"

CSVWorld::RecordStatusDelegate::RecordStatusDelegate(const ValueList& values,
                                                     const IconList & icons,
                                                     QUndoStack &undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack, parent)
{}

CSVWorld::CommandDelegate *CSVWorld::RecordStatusDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RecordStatusDelegate (mValues, mIcons, undoStack, parent);
}

bool CSVWorld::RecordStatusDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    if (settingName == "Record Status Display")
    {
        if (settingValue == "Icon and Text")
            mDisplayMode = Mode_IconAndText;

        else if (settingValue == "Icon Only")
            mDisplayMode = Mode_IconOnly;

        else if (settingValue == "Text Only")
            mDisplayMode = Mode_TextOnly;

        return true;
    }

    return false;
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
