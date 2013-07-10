#include "refidtypedelegate.hpp"
#include "../../model/world/universalid.hpp"

CSVWorld::RefIdTypeDelegate::RefIdTypeDelegate
    (const ValueList &values, const IconList &icons, QUndoStack& undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack, parent)
{}

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

    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Activator, ":./activator.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Potion, ":./potion.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Apparatus, ":./apparatus.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Armor, ":./armor.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Book, ":./book.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Clothing, ":./clothing.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Container, ":./container.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Creature, ":./creature.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Door, ":./door.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Ingredient, ":./ingredient.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_CreatureLevelledList, ":./creature.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_ItemLevelledList, ":./item.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Light, ":./light.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Lockpick, ":./lockpick.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Miscellaneous, ":./misc.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Npc, ":./npc.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Probe, ":./probe.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Repair, ":./repair.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Static, ":./static.png"));
    list.push_back (std::make_pair (CSMWorld::UniversalId::Type_Weapon, ":./weapon.png"));

    return list;
}

void CSVWorld::RefIdTypeDelegate::updateEditorSetting (const QString &settingName, const QString &settingValue)
{
    if (settingName == "Referenceable ID Type Display")
    {
        if (settingValue == "Icon and Text")
            mDisplayMode = Mode_IconAndText;

        else if (settingValue == "Icon Only")
            mDisplayMode = Mode_IconOnly;

        else if (settingValue == "Text Only")
            mDisplayMode = Mode_TextOnly;
    }
}
