#include "tablemimedata.hpp"
#include <string>

#include "universalid.hpp"
#include "columnbase.hpp"

CSMWorld::TableMimeData::TableMimeData (UniversalId id, const CSMDoc::Document& document) :
mDocument(document)
{
    mUniversalId.push_back (id);
    mObjectsFormats << QString::fromStdString ("tabledata/" + id.getTypeName());
}

CSMWorld::TableMimeData::TableMimeData (std::vector< CSMWorld::UniversalId >& id, const CSMDoc::Document& document) :
    mUniversalId (id), mDocument(document)
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

bool CSMWorld::TableMimeData::fromDocument (const CSMDoc::Document& document) const
{
    return &document == &mDocument;
}

CSMWorld::UniversalId::Type CSMWorld::TableMimeData::convertEnums (CSMWorld::ColumnBase::Display type)
{
    switch (type)
    {
        case CSMWorld::ColumnBase::Display_Race:
            return CSMWorld::UniversalId::Type_Race;


        case CSMWorld::ColumnBase::Display_Skill:
            return CSMWorld::UniversalId::Type_Skill;


        case CSMWorld::ColumnBase::Display_Class:
            return CSMWorld::UniversalId::Type_Class;


        case CSMWorld::ColumnBase::Display_Faction:
            return CSMWorld::UniversalId::Type_Faction;


        case CSMWorld::ColumnBase::Display_Sound:
            return CSMWorld::UniversalId::Type_Sound;


        case CSMWorld::ColumnBase::Display_Region:
            return CSMWorld::UniversalId::Type_Region;


        case CSMWorld::ColumnBase::Display_Birthsign:
            return CSMWorld::UniversalId::Type_Birthsign;


        case CSMWorld::ColumnBase::Display_Spell:
            return CSMWorld::UniversalId::Type_Spell;


        case CSMWorld::ColumnBase::Display_Cell:
            return CSMWorld::UniversalId::Type_Cell;


        case CSMWorld::ColumnBase::Display_Referenceable:
            return CSMWorld::UniversalId::Type_Referenceable;


        case CSMWorld::ColumnBase::Display_Activator:
            return CSMWorld::UniversalId::Type_Activator;


        case CSMWorld::ColumnBase::Display_Potion:
            return CSMWorld::UniversalId::Type_Potion;


        case CSMWorld::ColumnBase::Display_Apparatus:
            return CSMWorld::UniversalId::Type_Apparatus;


        case CSMWorld::ColumnBase::Display_Armor:
            return CSMWorld::UniversalId::Type_Armor;


        case CSMWorld::ColumnBase::Display_Book:
            return CSMWorld::UniversalId::Type_Book;


        case CSMWorld::ColumnBase::Display_Clothing:
            return CSMWorld::UniversalId::Type_Clothing;


        case CSMWorld::ColumnBase::Display_Container:
            return CSMWorld::UniversalId::Type_Container;


        case CSMWorld::ColumnBase::Display_Creature:
            return CSMWorld::UniversalId::Type_Creature;


        case CSMWorld::ColumnBase::Display_Door:
            return CSMWorld::UniversalId::Type_Door;


        case CSMWorld::ColumnBase::Display_Ingredient:
            return CSMWorld::UniversalId::Type_Ingredient;


        case CSMWorld::ColumnBase::Display_CreatureLevelledList:
            return CSMWorld::UniversalId::Type_CreatureLevelledList;


        case CSMWorld::ColumnBase::Display_ItemLevelledList:
            return CSMWorld::UniversalId::Type_ItemLevelledList;


        case CSMWorld::ColumnBase::Display_Light:
            return CSMWorld::UniversalId::Type_Light;


        case CSMWorld::ColumnBase::Display_Lockpick:
            return CSMWorld::UniversalId::Type_Lockpick;


        case CSMWorld::ColumnBase::Display_Miscellaneous:
            return CSMWorld::UniversalId::Type_Miscellaneous;


        case CSMWorld::ColumnBase::Display_Npc:
            return CSMWorld::UniversalId::Type_Npc;


        case CSMWorld::ColumnBase::Display_Probe:
            return CSMWorld::UniversalId::Type_Probe;


        case CSMWorld::ColumnBase::Display_Repair:
            return CSMWorld::UniversalId::Type_Repair;


        case CSMWorld::ColumnBase::Display_Static:
            return CSMWorld::UniversalId::Type_Static;


        case CSMWorld::ColumnBase::Display_Weapon:
            return CSMWorld::UniversalId::Type_Weapon;


        case CSMWorld::ColumnBase::Display_Reference:
            return CSMWorld::UniversalId::Type_Reference;


        case CSMWorld::ColumnBase::Display_Filter:
            return CSMWorld::UniversalId::Type_Filter;


        case CSMWorld::ColumnBase::Display_Topic:
            return CSMWorld::UniversalId::Type_Topic;


        case CSMWorld::ColumnBase::Display_Journal:
            return CSMWorld::UniversalId::Type_Journal;


        case CSMWorld::ColumnBase::Display_TopicInfo:
            return CSMWorld::UniversalId::Type_TopicInfo;


        case CSMWorld::ColumnBase::Display_JournalInfo:
            return CSMWorld::UniversalId::Type_JournalInfo;


        case CSMWorld::ColumnBase::Display_Scene:
            return CSMWorld::UniversalId::Type_Scene;


        case CSMWorld::ColumnBase::Display_Script:
            return CSMWorld::UniversalId::Type_Script;


        default:
            return CSMWorld::UniversalId::Type_None;

    }
}

CSMWorld::ColumnBase::Display CSMWorld::TableMimeData::convertEnums (CSMWorld::UniversalId::Type type)
{
    switch (type)
    {
        case CSMWorld::UniversalId::Type_Race:
            return CSMWorld::ColumnBase::Display_Race;


        case CSMWorld::UniversalId::Type_Skill:
            return CSMWorld::ColumnBase::Display_Skill;


        case CSMWorld::UniversalId::Type_Class:
            return CSMWorld::ColumnBase::Display_Class;


        case CSMWorld::UniversalId::Type_Faction:
            return CSMWorld::ColumnBase::Display_Faction;


        case CSMWorld::UniversalId::Type_Sound:
            return CSMWorld::ColumnBase::Display_Sound;


        case CSMWorld::UniversalId::Type_Region:
            return CSMWorld::ColumnBase::Display_Region;


        case CSMWorld::UniversalId::Type_Birthsign:
            return CSMWorld::ColumnBase::Display_Birthsign;


        case CSMWorld::UniversalId::Type_Spell:
            return CSMWorld::ColumnBase::Display_Spell;


        case CSMWorld::UniversalId::Type_Cell:
            return CSMWorld::ColumnBase::Display_Cell;


        case CSMWorld::UniversalId::Type_Referenceable:
            return CSMWorld::ColumnBase::Display_Referenceable;


        case CSMWorld::UniversalId::Type_Activator:
            return CSMWorld::ColumnBase::Display_Activator;


        case CSMWorld::UniversalId::Type_Potion:
            return CSMWorld::ColumnBase::Display_Potion;


        case CSMWorld::UniversalId::Type_Apparatus:
            return CSMWorld::ColumnBase::Display_Apparatus;


        case CSMWorld::UniversalId::Type_Armor:
            return CSMWorld::ColumnBase::Display_Armor;


        case CSMWorld::UniversalId::Type_Book:
            return CSMWorld::ColumnBase::Display_Book;


        case CSMWorld::UniversalId::Type_Clothing:
            return CSMWorld::ColumnBase::Display_Clothing;


        case CSMWorld::UniversalId::Type_Container:
            return CSMWorld::ColumnBase::Display_Container;


        case CSMWorld::UniversalId::Type_Creature:
            return CSMWorld::ColumnBase::Display_Creature;


        case CSMWorld::UniversalId::Type_Door:
            return CSMWorld::ColumnBase::Display_Door;


        case CSMWorld::UniversalId::Type_Ingredient:
            return CSMWorld::ColumnBase::Display_Ingredient;


        case CSMWorld::UniversalId::Type_CreatureLevelledList:
            return CSMWorld::ColumnBase::Display_CreatureLevelledList;


        case CSMWorld::UniversalId::Type_ItemLevelledList:
            return CSMWorld::ColumnBase::Display_ItemLevelledList;


        case CSMWorld::UniversalId::Type_Light:
            return CSMWorld::ColumnBase::Display_Light;


        case CSMWorld::UniversalId::Type_Lockpick:
            return CSMWorld::ColumnBase::Display_Lockpick;


        case CSMWorld::UniversalId::Type_Miscellaneous:
            return CSMWorld::ColumnBase::Display_Miscellaneous;


        case CSMWorld::UniversalId::Type_Npc:
            return CSMWorld::ColumnBase::Display_Npc;


        case CSMWorld::UniversalId::Type_Probe:
            return CSMWorld::ColumnBase::Display_Probe;


        case CSMWorld::UniversalId::Type_Repair:
            return CSMWorld::ColumnBase::Display_Repair;


        case CSMWorld::UniversalId::Type_Static:
            return CSMWorld::ColumnBase::Display_Static;


        case CSMWorld::UniversalId::Type_Weapon:
            return CSMWorld::ColumnBase::Display_Weapon;


        case CSMWorld::UniversalId::Type_Reference:
            return CSMWorld::ColumnBase::Display_Reference;


        case CSMWorld::UniversalId::Type_Filter:
            return CSMWorld::ColumnBase::Display_Filter;


        case CSMWorld::UniversalId::Type_Topic:
            return CSMWorld::ColumnBase::Display_Topic;


        case CSMWorld::UniversalId::Type_Journal:
            return CSMWorld::ColumnBase::Display_Journal;


        case CSMWorld::UniversalId::Type_TopicInfo:
            return CSMWorld::ColumnBase::Display_TopicInfo;


        case CSMWorld::UniversalId::Type_JournalInfo:
            return CSMWorld::ColumnBase::Display_JournalInfo;


        case CSMWorld::UniversalId::Type_Scene:
            return CSMWorld::ColumnBase::Display_Scene;


        case CSMWorld::UniversalId::Type_Script:
            return CSMWorld::ColumnBase::Display_Script;


        default:
            return CSMWorld::ColumnBase::Display_None;
    }
}