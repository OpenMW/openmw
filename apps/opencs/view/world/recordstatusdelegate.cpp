#include "recordstatusdelegate.hpp"
#include <QPainter>
#include <QApplication>
#include <QUndoStack>
#include "../../model/settings/usersettings.hpp"

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

void CSVWorld::RecordStatusDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    if (settingName == "Record Status Display")
    {
        if (settingValue == "Icon and Text")
            mDisplayMode = Mode_IconAndText;

        else if (settingValue == "Icon Only")
            mDisplayMode = Mode_IconOnly;

        else if (settingValue == "Text Only")
            mDisplayMode = Mode_TextOnly;
    }
}

CSVWorld::RecordStatusDelegateFactory::RecordStatusDelegateFactory()
{
    DataDisplayDelegateFactory::add ( CSMWorld::RecordBase::State_BaseOnly,     "Base",     ":./base.png");
    DataDisplayDelegateFactory::add ( CSMWorld::RecordBase::State_Deleted,      "Deleted",  ":./removed.png");
    DataDisplayDelegateFactory::add ( CSMWorld::RecordBase::State_Erased,       "Deleted",  ":./removed.png");
    DataDisplayDelegateFactory::add ( CSMWorld::RecordBase::State_Modified,     "Modified", ":./modified.png");
    DataDisplayDelegateFactory::add ( CSMWorld::RecordBase::State_ModifiedOnly, "Added",    ":./added.png");
}
