
#include "universalid.hpp"

#include <ostream>
#include <stdexcept>
#include <sstream>

namespace
{
    struct TypeData
    {
            CSMWorld::UniversalId::Class mClass;
            CSMWorld::UniversalId::Type mType;
            const char *mName;
            const char *mIcon;
    };

    static const TypeData sNoArg[] =
    {
        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, "empty", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Globals, "Global Variables", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Gmsts, "Game Settings", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Skills, "Skills", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Classes, "Classes", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Factions, "Factions", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Races, "Races", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Sounds, "Sounds", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Scripts, "Scripts", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Regions, "Regions", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Birthsigns, "Birthsigns", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Spells, "Spells", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Topics, "Topics", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Journals, "Journals", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Cells, "Cells", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Referenceables,
            "Referenceables", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_References,
            "References", 0 },
        { CSMWorld::UniversalId::Class_NonRecord, CSMWorld::UniversalId::Type_RegionMap,
            "Region Map", 0 },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Filters, "Filters", 0 },

        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0, 0 } // end marker
    };

    static const TypeData sIdArg[] =
    {
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Global, "Global Variable", ":./globvar.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Gmst, "Game Setting", ":./GMST.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Skill, "Skill", ":./skill.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Class, "Class", ":./class.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Faction, "Faction", ":./faction.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Race, "Race", ":./race.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Sound, "Sound", ":./sound.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Script, "Script", ":./script.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Region, "Region", ":./land.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Birthsign, "Birthsign", ":./birthsign.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Spell, "Spell", ":./spell.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Topic, "Topic", 0 },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Journal, "Journal", 0 },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Cell, "Cell", ":./cell.png" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Referenceable, "Referenceables", 0 },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Activator, "Activator", ":./activator.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Potion, "Potion", ":./potion.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Apparatus, "Apparatus", ":./apparatus.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Armor, "Armor", ":./armor.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Book, "Book", ":./book.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Clothing, "Clothing", ":./clothing.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Container, "Container", ":./container.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Creature, "Creature", ":./creature.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Door, "Door", ":./door.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Ingredient, "Ingredient", ":./ingredient.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_CreatureLevelledList,
            "Creature Levelled List", ":./creature.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_ItemLevelledList,
            "Item Levelled List", ":./leveled-item.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Light, "Light", ":./light.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Lockpick, "Lockpick", ":./lockpick.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Miscellaneous,
            "Miscellaneous", ":./miscellaneous.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Npc, "NPC", ":./npc.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Probe, "Probe", ":./probe.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Repair, "Repair", ":./repair.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Static, "Static", ":./static.png" },
        { CSMWorld::UniversalId::Class_RefRecord, CSMWorld::UniversalId::Type_Weapon, "Weapon", ":./weapon.png" },
        { CSMWorld::UniversalId::Class_SubRecord, CSMWorld::UniversalId::Type_Reference, "Reference", 0 },
        { CSMWorld::UniversalId::Class_SubRecord, CSMWorld::UniversalId::Type_Filter, "Filter", ":./filter.png" },
        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0, 0 } // end marker
    };

    static const TypeData sIndexArg[] =
    {
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_VerificationResults, "Verification Results", 0 },
        { CSMWorld::UniversalId::Class_Collection, CSMWorld::UniversalId::Type_Scene, "Scene", 0 },

        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0, 0 } // end marker
    };
}

CSMWorld::UniversalId::UniversalId (const std::string& universalId)
{
    std::string::size_type index = universalId.find (':');

    if (index!=std::string::npos)
    {
        std::string type = universalId.substr (0, index);

        for (int i=0; sIdArg[i].mName; ++i)
            if (type==sIdArg[i].mName)
            {
                mArgumentType = ArgumentType_Id;
                mType = sIdArg[i].mType;
                mClass = sIdArg[i].mClass;
                mId = universalId.substr (index+2);
                return;
            }

        for (int i=0; sIndexArg[i].mName; ++i)
            if (type==sIndexArg[i].mName)
            {
                mArgumentType = ArgumentType_Index;
                mType = sIndexArg[i].mType;
                mClass = sIndexArg[i].mClass;

                std::istringstream stream (universalId.substr (index+2));

                if (stream >> mIndex)
                    return;

                break;
            }
    }
    else
    {
        for (int i=0; sNoArg[i].mName; ++i)
            if (universalId==sNoArg[i].mName)
            {
                mArgumentType = ArgumentType_None;
                mType = sNoArg[i].mType;
                mClass = sNoArg[i].mClass;
                return;
            }
    }

    throw std::runtime_error ("invalid UniversalId: " + universalId);
}

CSMWorld::UniversalId::UniversalId (Type type) : mArgumentType (ArgumentType_None), mType (type), mIndex (0)
{
    for (int i=0; sNoArg[i].mName; ++i)
        if (type==sNoArg[i].mType)
        {
            mClass = sNoArg[i].mClass;
            return;
        }

    for (int i=0; sIdArg[i].mName; ++i)
        if (type==sIdArg[i].mType)
        {
            mArgumentType = ArgumentType_Id;
            mClass = sIdArg[i].mClass;
            return;
        }

    for (int i=0; sIndexArg[i].mName; ++i)
        if (type==sIndexArg[i].mType)
        {
            mArgumentType = ArgumentType_Index;
            mClass = sIndexArg[i].mClass;
            return;
        }

    throw std::logic_error ("invalid argument-less UniversalId type");
}

CSMWorld::UniversalId::UniversalId (Type type, const std::string& id)
: mArgumentType (ArgumentType_Id), mType (type), mId (id), mIndex (0)
{
    for (int i=0; sIdArg[i].mName; ++i)
        if (type==sIdArg[i].mType)
        {
            mClass = sIdArg[i].mClass;
            return;
        }

    throw std::logic_error ("invalid ID argument UniversalId type");
}

CSMWorld::UniversalId::UniversalId (Type type, int index)
: mArgumentType (ArgumentType_Index), mType (type), mIndex (index)
{
    for (int i=0; sIndexArg[i].mName; ++i)
        if (type==sIndexArg[i].mType)
        {
            mClass = sIndexArg[i].mClass;
            return;
        }

    throw std::logic_error ("invalid index argument UniversalId type");
}

CSMWorld::UniversalId::Class CSMWorld::UniversalId::getClass() const
{
    return mClass;
}

CSMWorld::UniversalId::ArgumentType CSMWorld::UniversalId::getArgumentType() const
{
    return mArgumentType;
}

CSMWorld::UniversalId::Type CSMWorld::UniversalId::getType() const
{
    return mType;
}

const std::string& CSMWorld::UniversalId::getId() const
{
    if (mArgumentType!=ArgumentType_Id)
        throw std::logic_error ("invalid access to ID of non-ID UniversalId");

    return mId;
}

int CSMWorld::UniversalId::getIndex() const
{
    if (mArgumentType!=ArgumentType_Index)
        throw std::logic_error ("invalid access to index of non-index UniversalId");

    return mIndex;
}

bool CSMWorld::UniversalId::isEqual (const UniversalId& universalId) const
{
    if (mClass!=universalId.mClass || mArgumentType!=universalId.mArgumentType || mType!=universalId.mType)
            return false;

    switch (mArgumentType)
    {
        case ArgumentType_Id: return mId==universalId.mId;
        case ArgumentType_Index: return mIndex==universalId.mIndex;

        default: return true;
    }
}

bool CSMWorld::UniversalId::isLess (const UniversalId& universalId) const
{
    if (mType<universalId.mType)
        return true;

    if (mType>universalId.mType)
        return false;

    switch (mArgumentType)
    {
        case ArgumentType_Id: return mId<universalId.mId;
        case ArgumentType_Index: return mIndex<universalId.mIndex;

        default: return false;
    }
}

std::string CSMWorld::UniversalId::getTypeName() const
{
    const TypeData *typeData = mArgumentType==ArgumentType_None ? sNoArg :
        (mArgumentType==ArgumentType_Id ? sIdArg : sIndexArg);

    for (int i=0; typeData[i].mName; ++i)
        if (typeData[i].mType==mType)
            return typeData[i].mName;

    throw std::logic_error ("failed to retrieve UniversalId type name");
}

std::string CSMWorld::UniversalId::toString() const
{
    std::ostringstream stream;

    stream << getTypeName();

    switch (mArgumentType)
    {
        case ArgumentType_None: break;
        case ArgumentType_Id: stream << ": " << mId; break;
        case ArgumentType_Index: stream << ": " << mIndex; break;
    }

    return stream.str();
}

std::string CSMWorld::UniversalId::getIcon() const
{
    const TypeData *typeData = mArgumentType==ArgumentType_None ? sNoArg :
        (mArgumentType==ArgumentType_Id ? sIdArg : sIndexArg);

    for (int i=0; typeData[i].mName; ++i)
        if (typeData[i].mType==mType)
            return typeData[i].mIcon ? typeData[i].mIcon : "";

    throw std::logic_error ("failed to retrieve UniversalId type icon");
}

std::vector<CSMWorld::UniversalId::Type> CSMWorld::UniversalId::listReferenceableTypes()
{
    std::vector<CSMWorld::UniversalId::Type> list;

    for (int i=0; sIdArg[i].mName; ++i)
        if (sIdArg[i].mClass==Class_RefRecord)
            list.push_back (sIdArg[i].mType);

    return list;
}

bool CSMWorld::operator== (const CSMWorld::UniversalId& left, const CSMWorld::UniversalId& right)
{
    return left.isEqual (right);
}

bool CSMWorld::operator!= (const CSMWorld::UniversalId& left, const CSMWorld::UniversalId& right)
{
    return !left.isEqual (right);
}

bool CSMWorld::operator< (const UniversalId& left, const UniversalId& right)
{
    return left.isLess (right);
}

std::ostream& CSMWorld::operator< (std::ostream& stream, const CSMWorld::UniversalId& universalId)
{
    return stream << universalId.toString();
}
