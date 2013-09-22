#include "idtypedelegate.hpp"

#include "../../model/world/universalid.hpp"

CSVWorld::IdTypeDelegate::IdTypeDelegate
    (const ValueList &values, const IconList &icons, QUndoStack& undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack, parent)
{}

bool CSVWorld::IdTypeDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    /// \todo make the setting key a member variable, that is initialised from a constructor argument
    if (settingName == "Referenceable ID Type Display")
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


CSVWorld::IdTypeDelegateFactory::IdTypeDelegateFactory()
{
    for (int i=0; i<CSMWorld::UniversalId::NumberOfTypes; ++i)
    {
        CSMWorld::UniversalId id (static_cast<CSMWorld::UniversalId::Type> (i));

        DataDisplayDelegateFactory::add (id.getType(), QString::fromUtf8 (id.getTypeName().c_str()),
            QString::fromUtf8 (id.getIcon().c_str()));
    }
}

CSVWorld::CommandDelegate *CSVWorld::IdTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new IdTypeDelegate (mValues, mIcons, undoStack, parent);
}
