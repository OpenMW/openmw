#include "tablemimedata.hpp"
#include <string>

#include "universalid.hpp"
#include "columnbase.hpp"

CSMWorld::TableMimeData::TableMimeData (UniversalId id)
{
    mUniversalId.push_back (id);
    mObjectsFormats << QString::fromStdString ("tabledata/" + id.getTypeName());
}

CSMWorld::TableMimeData::TableMimeData (std::vector< CSMWorld::UniversalId >& id) :
    mUniversalId (id)
{
    for (std::vector<UniversalId>::iterator it (mUniversalId.begin()); it != mUniversalId.end(); ++it)
    {
        mObjectsFormats << QString::fromStdString ("tabledata/" + it->getTypeName());
    }
}

QStringList CSMWorld::TableMimeData::formats() const
{
    return mObjectsFormats;
}

CSMWorld::TableMimeData::~TableMimeData()
{
}

std::string CSMWorld::TableMimeData::getIcon() const
{
    if (mUniversalId.empty())
    {
        throw ("TableMimeData holds no UniversalId");
    }

    std::string tmpIcon;
    bool firstIteration = true;

    for (unsigned i = 0; i < mUniversalId.size(); ++i)
    {
        if (firstIteration)
        {
            firstIteration = false;
            tmpIcon = mUniversalId[i].getIcon();
            continue;
        }

        if (tmpIcon != mUniversalId[i].getIcon())
        {
            return ":/multitype.png"; //icon stolen from gnome
        }

        tmpIcon = mUniversalId[i].getIcon();
    }

    return mUniversalId.begin()->getIcon(); //All objects are of the same type;
}

std::vector< CSMWorld::UniversalId > CSMWorld::TableMimeData::getData() const
{
    return mUniversalId;
}

bool CSMWorld::TableMimeData::holdsType (CSMWorld::UniversalId::Type type) const
{
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (it->getType() == type)
        {
            return true;
        }
    }

    return false;
}

bool CSMWorld::TableMimeData::holdsType (CSMWorld::ColumnBase::Display type) const
{
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (it->getType() == convertEnums (type))
        {
            return true;
        }
    }

    return false;
}

CSMWorld::UniversalId CSMWorld::TableMimeData::returnMatching (CSMWorld::UniversalId::Type type) const
{
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (it->getType() == type)
        {
            return *it;
        }
    }

    throw ("TableMimeData object does not hold object of the seeked type");
}

CSMWorld::UniversalId CSMWorld::TableMimeData::returnMatching (CSMWorld::ColumnBase::Display type) const
{
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (it->getType() == convertEnums (type))
        {
            return *it;
        }
    }

    throw ("TableMimeData object does not hold object of the seeked type");
}

CSMWorld::UniversalId::Type CSMWorld::TableMimeData::convertEnums (CSMWorld::ColumnBase::Display type) const
{
    switch (type)
    {
        case CSMWorld::ColumnBase::Display_Race:
            return CSMWorld::UniversalId::Type_Race;
            break;

        case CSMWorld::ColumnBase::Display_Skill:
            return CSMWorld::UniversalId::Type_Skill;
            break;

        case CSMWorld::ColumnBase::Display_Class:
            return CSMWorld::UniversalId::Type_Class;
            break;

        case CSMWorld::ColumnBase::Display_Faction:
            return CSMWorld::UniversalId::Type_Faction;
            break;

        case CSMWorld::ColumnBase::Display_Sound:
            return CSMWorld::UniversalId::Type_Sound;
            break;

        case CSMWorld::ColumnBase::Display_Region:
            return CSMWorld::UniversalId::Type_Region;
            break;

        case CSMWorld::ColumnBase::Display_Birthsign:
            return CSMWorld::UniversalId::Type_Birthsign;
            break;

        case CSMWorld::ColumnBase::Display_Spell:
            return CSMWorld::UniversalId::Type_Spell;
            break;

        case CSMWorld::ColumnBase::Display_Cell:
            return CSMWorld::UniversalId::Type_Cell;
            break;

        case CSMWorld::ColumnBase::Display_Referenceable:
            return CSMWorld::UniversalId::Type_Referenceable;
            break;

        case CSMWorld::ColumnBase::Display_Activator:
            return CSMWorld::UniversalId::Type_Activator;
            break;

        case CSMWorld::ColumnBase::Display_Potion:
            return CSMWorld::UniversalId::Type_Potion;
            break;

        case CSMWorld::ColumnBase::Display_Apparatus:
            return CSMWorld::UniversalId::Type_Apparatus;
            break;

        case CSMWorld::ColumnBase::Display_Armor:
            return CSMWorld::UniversalId::Type_Armor;
            break;

        case CSMWorld::ColumnBase::Display_Book:
            return CSMWorld::UniversalId::Type_Book;
            break;

        case CSMWorld::ColumnBase::Display_Clothing:
            return CSMWorld::UniversalId::Type_Clothing;
            break;

        case CSMWorld::ColumnBase::Display_Container:
            return CSMWorld::UniversalId::Type_Container;
            break;

        case CSMWorld::ColumnBase::Display_Creature:
            return CSMWorld::UniversalId::Type_Creature;
            break;

        case CSMWorld::ColumnBase::Display_Door:
            return CSMWorld::UniversalId::Type_Door;
            break;

        case CSMWorld::ColumnBase::Display_Ingredient:
            return CSMWorld::UniversalId::Type_Ingredient;
            break;

        case CSMWorld::ColumnBase::Display_CreatureLevelledList:
            return CSMWorld::UniversalId::Type_CreatureLevelledList;
            break;

        case CSMWorld::ColumnBase::Display_ItemLevelledList:
            return CSMWorld::UniversalId::Type_ItemLevelledList;
            break;

        case CSMWorld::ColumnBase::Display_Light:
            return CSMWorld::UniversalId::Type_Light;
            break;

        case CSMWorld::ColumnBase::Display_Lockpick:
            return CSMWorld::UniversalId::Type_Lockpick;
            break;

        case CSMWorld::ColumnBase::Display_Miscellaneous:
            return CSMWorld::UniversalId::Type_Miscellaneous;
            break;

        case CSMWorld::ColumnBase::Display_Npc:
            return CSMWorld::UniversalId::Type_Npc;
            break;

        case CSMWorld::ColumnBase::Display_Probe:
            return CSMWorld::UniversalId::Type_Probe;
            break;

        case CSMWorld::ColumnBase::Display_Repair:
            return CSMWorld::UniversalId::Type_Repair;
            break;

        case CSMWorld::ColumnBase::Display_Static:
            return CSMWorld::UniversalId::Type_Static;
            break;

        case CSMWorld::ColumnBase::Display_Weapon:
            return CSMWorld::UniversalId::Type_Weapon;
            break;

        case CSMWorld::ColumnBase::Display_Reference:
            return CSMWorld::UniversalId::Type_Reference;
            break;

        case CSMWorld::ColumnBase::Display_Filter:
            return CSMWorld::UniversalId::Type_Filter;
            break;

        case CSMWorld::ColumnBase::Display_Topic:
            return CSMWorld::UniversalId::Type_Topic;
            break;

        case CSMWorld::ColumnBase::Display_Journal:
            return CSMWorld::UniversalId::Type_Journal;
            break;

        case CSMWorld::ColumnBase::Display_TopicInfo:
            return CSMWorld::UniversalId::Type_TopicInfo;
            break;

        case CSMWorld::ColumnBase::Display_JournalInfo:
            return CSMWorld::UniversalId::Type_JournalInfo;
            break;

        case CSMWorld::ColumnBase::Display_Scene:
            return CSMWorld::UniversalId::Type_Scene;
            break;

        case CSMWorld::ColumnBase::Display_Script:
            return CSMWorld::UniversalId::Type_Script;
            break;

        default:
            return CSMWorld::UniversalId::Type_None;
            break;
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
