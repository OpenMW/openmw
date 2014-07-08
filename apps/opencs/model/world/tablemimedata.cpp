#include "tablemimedata.hpp"

#include <string>

#include <QDebug>

#include "universalid.hpp"
#include "columnbase.hpp"

CSMWorld::TableMimeData::TableMimeData (UniversalId id, const CSMDoc::Document& document) :
mDocument(document)
{
    mUniversalId.push_back (id);
    mObjectsFormats << QString::fromUtf8 (("tabledata/" + id.getTypeName()).c_str());
}

CSMWorld::TableMimeData::TableMimeData (const std::vector< CSMWorld::UniversalId >& id, const CSMDoc::Document& document) :
    mUniversalId (id), mDocument(document)
{
    for (std::vector<UniversalId>::iterator it (mUniversalId.begin()); it != mUniversalId.end(); ++it)
    {
        mObjectsFormats << QString::fromUtf8 (("tabledata/" + it->getTypeName()).c_str());
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
        qDebug()<<"TableMimeData object does not hold any records!"; //because throwing in the event loop tends to be problematic
        throw("TableMimeData object does not hold any records!");
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
            return ":/multitype.png"; //icon stolen from gnome TODO: get new icon
        }

        tmpIcon = mUniversalId[i].getIcon();
    }

    return mUniversalId.begin()->getIcon(); //All objects are of the same type;
}

std::vector< CSMWorld::UniversalId > CSMWorld::TableMimeData::getData() const
{
    return mUniversalId;
}

bool CSMWorld::TableMimeData::isReferencable(CSMWorld::ColumnBase::Display type) const
{
return (  type == CSMWorld::ColumnBase::Display_Activator
       || type == CSMWorld::ColumnBase::Display_Potion
       || type == CSMWorld::ColumnBase::Display_Apparatus
       || type == CSMWorld::ColumnBase::Display_Armor
       || type == CSMWorld::ColumnBase::Display_Book
       || type == CSMWorld::ColumnBase::Display_Clothing
       || type == CSMWorld::ColumnBase::Display_Container
       || type == CSMWorld::ColumnBase::Display_Creature
       || type == CSMWorld::ColumnBase::Display_Door
       || type == CSMWorld::ColumnBase::Display_Ingredient
       || type == CSMWorld::ColumnBase::Display_CreatureLevelledList
       || type == CSMWorld::ColumnBase::Display_ItemLevelledList
       || type == CSMWorld::ColumnBase::Display_Light
       || type == CSMWorld::ColumnBase::Display_Lockpick
       || type == CSMWorld::ColumnBase::Display_Miscellaneous
       || type == CSMWorld::ColumnBase::Display_Npc
       || type == CSMWorld::ColumnBase::Display_Probe
       || type == CSMWorld::ColumnBase::Display_Repair
       || type == CSMWorld::ColumnBase::Display_Static
       || type == CSMWorld::ColumnBase::Display_Weapon);
}
bool CSMWorld::TableMimeData::isReferencable(CSMWorld::UniversalId::Type type) const
{
     return (  type == CSMWorld::UniversalId::Type_Activator
            || type == CSMWorld::UniversalId::Type_Potion
            || type == CSMWorld::UniversalId::Type_Apparatus
            || type == CSMWorld::UniversalId::Type_Armor
            || type == CSMWorld::UniversalId::Type_Book
            || type == CSMWorld::UniversalId::Type_Clothing
            || type == CSMWorld::UniversalId::Type_Container
            || type == CSMWorld::UniversalId::Type_Creature
            || type == CSMWorld::UniversalId::Type_Door
            || type == CSMWorld::UniversalId::Type_Ingredient
            || type == CSMWorld::UniversalId::Type_CreatureLevelledList
            || type == CSMWorld::UniversalId::Type_ItemLevelledList
            || type == CSMWorld::UniversalId::Type_Light
            || type == CSMWorld::UniversalId::Type_Lockpick
            || type == CSMWorld::UniversalId::Type_Miscellaneous
            || type == CSMWorld::UniversalId::Type_Npc
            || type == CSMWorld::UniversalId::Type_Probe
            || type == CSMWorld::UniversalId::Type_Repair
            || type == CSMWorld::UniversalId::Type_Static
            || type == CSMWorld::UniversalId::Type_Weapon);
}

bool CSMWorld::TableMimeData::holdsType (CSMWorld::UniversalId::Type type) const
{
    bool referencable = (type == CSMWorld::UniversalId::Type_Referenceable);
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (referencable)
        {
            if (isReferencable(it->getType()))
            {
                return true;
            }
        } else {
        if (it->getType() == type)
            {
                return true;
            }
        }
    }

    return false;
}

bool CSMWorld::TableMimeData::holdsType (CSMWorld::ColumnBase::Display type) const
{
    bool referencable = (type == CSMWorld::ColumnBase::Display_Referenceable);
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (referencable)
        {
            if (isReferencable(it->getType()))
            {
                return true;
            }
        } else {
        if (it->getType() == convertEnums (type))
            {
                return true;
            }
        }
    }

    return false;
}

CSMWorld::UniversalId CSMWorld::TableMimeData::returnMatching (CSMWorld::UniversalId::Type type) const
{
    bool referencable = (type == CSMWorld::UniversalId::Type_Referenceable);
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (referencable)
        {
            if (isReferencable(it->getType()))
            {
                return *it;
            }
        } else
        {
            if (it->getType() == type)
            {
                return *it;
            }
        }
    }

    throw ("TableMimeData object does not hold object of the seeked type");
}

CSMWorld::UniversalId CSMWorld::TableMimeData::returnMatching (CSMWorld::ColumnBase::Display type) const
{
    bool referencable = (type == CSMWorld::ColumnBase::Display_Referenceable);
    for (std::vector<UniversalId>::const_iterator it = mUniversalId.begin(); it != mUniversalId.end(); ++it)
    {
        if (referencable)
        {
            if (isReferencable(it->getType()))
            {
                return *it;
            }
        } else {
            if (it->getType() == convertEnums (type))
            {
                return *it;
            }
        }
    }

    throw ("TableMimeData object does not hold object of the seeked type");
}

bool CSMWorld::TableMimeData::fromDocument (const CSMDoc::Document& document) const
{
    return &document == &mDocument;
}

namespace
{
    struct Mapping
    {
        CSMWorld::UniversalId::Type mUniversalIdType;
        CSMWorld::ColumnBase::Display mDisplayType;
    };

    const Mapping mapping[] =
    {
        { CSMWorld::UniversalId::Type_Race, CSMWorld::ColumnBase::Display_Race },
        { CSMWorld::UniversalId::Type_Skill, CSMWorld::ColumnBase::Display_Skill },
        { CSMWorld::UniversalId::Type_Class, CSMWorld::ColumnBase::Display_Class },
        { CSMWorld::UniversalId::Type_Class, CSMWorld::ColumnBase::Display_Class },
        { CSMWorld::UniversalId::Type_Faction, CSMWorld::ColumnBase::Display_Faction },
        { CSMWorld::UniversalId::Type_Sound, CSMWorld::ColumnBase::Display_Sound },
        { CSMWorld::UniversalId::Type_Region, CSMWorld::ColumnBase::Display_Region },
        { CSMWorld::UniversalId::Type_Birthsign, CSMWorld::ColumnBase::Display_Birthsign },
        { CSMWorld::UniversalId::Type_Spell, CSMWorld::ColumnBase::Display_Spell },
        { CSMWorld::UniversalId::Type_Cell, CSMWorld::ColumnBase::Display_Cell },
        { CSMWorld::UniversalId::Type_Referenceable, CSMWorld::ColumnBase::Display_Referenceable },
        { CSMWorld::UniversalId::Type_Activator, CSMWorld::ColumnBase::Display_Activator },
        { CSMWorld::UniversalId::Type_Potion, CSMWorld::ColumnBase::Display_Potion },
        { CSMWorld::UniversalId::Type_Apparatus, CSMWorld::ColumnBase::Display_Apparatus },
        { CSMWorld::UniversalId::Type_Armor, CSMWorld::ColumnBase::Display_Armor },
        { CSMWorld::UniversalId::Type_Book, CSMWorld::ColumnBase::Display_Book },
        { CSMWorld::UniversalId::Type_Clothing, CSMWorld::ColumnBase::Display_Clothing },
        { CSMWorld::UniversalId::Type_Container, CSMWorld::ColumnBase::Display_Container },
        { CSMWorld::UniversalId::Type_Creature, CSMWorld::ColumnBase::Display_Creature },
        { CSMWorld::UniversalId::Type_Door, CSMWorld::ColumnBase::Display_Door },
        { CSMWorld::UniversalId::Type_Ingredient, CSMWorld::ColumnBase::Display_Ingredient },
        { CSMWorld::UniversalId::Type_CreatureLevelledList, CSMWorld::ColumnBase::Display_CreatureLevelledList },
        { CSMWorld::UniversalId::Type_ItemLevelledList, CSMWorld::ColumnBase::Display_ItemLevelledList },
        { CSMWorld::UniversalId::Type_Light, CSMWorld::ColumnBase::Display_Light },
        { CSMWorld::UniversalId::Type_Lockpick, CSMWorld::ColumnBase::Display_Lockpick },
        { CSMWorld::UniversalId::Type_Miscellaneous, CSMWorld::ColumnBase::Display_Miscellaneous },
        { CSMWorld::UniversalId::Type_Npc, CSMWorld::ColumnBase::Display_Npc },
        { CSMWorld::UniversalId::Type_Probe, CSMWorld::ColumnBase::Display_Probe },
        { CSMWorld::UniversalId::Type_Repair, CSMWorld::ColumnBase::Display_Repair },
        { CSMWorld::UniversalId::Type_Static, CSMWorld::ColumnBase::Display_Static },
        { CSMWorld::UniversalId::Type_Weapon, CSMWorld::ColumnBase::Display_Weapon },
        { CSMWorld::UniversalId::Type_Reference, CSMWorld::ColumnBase::Display_Reference },
        { CSMWorld::UniversalId::Type_Filter, CSMWorld::ColumnBase::Display_Filter },
        { CSMWorld::UniversalId::Type_Topic, CSMWorld::ColumnBase::Display_Topic },
        { CSMWorld::UniversalId::Type_Journal, CSMWorld::ColumnBase::Display_Journal },
        { CSMWorld::UniversalId::Type_TopicInfo, CSMWorld::ColumnBase::Display_TopicInfo },
        { CSMWorld::UniversalId::Type_JournalInfo, CSMWorld::ColumnBase::Display_JournalInfo },
        { CSMWorld::UniversalId::Type_Scene, CSMWorld::ColumnBase::Display_Scene },
        { CSMWorld::UniversalId::Type_Script, CSMWorld::ColumnBase::Display_Script },


        { CSMWorld::UniversalId::Type_None, CSMWorld::ColumnBase::Display_None } // end marker
    };
}

CSMWorld::UniversalId::Type CSMWorld::TableMimeData::convertEnums (ColumnBase::Display type)
{
    for (int i=0; mapping[i].mUniversalIdType!=CSMWorld::UniversalId::Type_None; ++i)
        if (mapping[i].mDisplayType==type)
            return mapping[i].mUniversalIdType;

    return CSMWorld::UniversalId::Type_None;
}

CSMWorld::ColumnBase::Display CSMWorld::TableMimeData::convertEnums (UniversalId::Type type)
{
    for (int i=0; mapping[i].mUniversalIdType!=CSMWorld::UniversalId::Type_None; ++i)
        if (mapping[i].mUniversalIdType==type)
            return mapping[i].mDisplayType;

    return CSMWorld::ColumnBase::Display_None;


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

const CSMDoc::Document* CSMWorld::TableMimeData::getDocumentPtr() const
{
    return &mDocument;
}
