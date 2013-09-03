#include "refidtypedelegate.hpp"

#include "../../model/world/universalid.hpp"

CSVWorld::RefIdTypeDelegate::RefIdTypeDelegate
    (const ValueList &values, const IconList &icons, QUndoStack& undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack, parent)
{}

bool CSVWorld::RefIdTypeDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
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


CSVWorld::RefIdTypeDelegateFactory::RefIdTypeDelegateFactory()
{
    UidTypeList uIdList = buildUidTypeList();

    for (UidTypeList::const_iterator it = uIdList.begin(); it != uIdList.end(); it++)
    {
        int i = it->first;
        DataDisplayDelegateFactory::add (i, QString::fromStdString(CSMWorld::UniversalId(it->first, "").getTypeName()), it->second);
    }
}

CSVWorld::CommandDelegate *CSVWorld::RefIdTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RefIdTypeDelegate (mValues, mIcons, undoStack, parent);
}

CSVWorld::RefIdTypeDelegateFactory::UidTypeList CSVWorld::RefIdTypeDelegateFactory::buildUidTypeList() const
{
    UidTypeList list;

    std::vector<CSMWorld::UniversalId::Type> types = CSMWorld::UniversalId::listReferenceableTypes();

    for (std::vector<CSMWorld::UniversalId::Type>::const_iterator iter (types.begin());
         iter!=types.end(); ++iter)
    {
        CSMWorld::UniversalId id (*iter, "");

        list.push_back (std::make_pair (id.getType(), id.getIcon().c_str()));
    }

    return list;
}
