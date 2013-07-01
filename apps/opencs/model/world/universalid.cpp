
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
    };

    static const TypeData sNoArg[] =
    {
        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, "empty" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Globals, "Global Variables" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Gmsts, "Game Settings" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Skills, "Skills" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Classes, "Classes" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Factions, "Factions" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Races, "Races" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Sounds, "Sounds" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Scripts, "Scripts" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Regions, "Regions" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Birthsigns, "Birthsigns" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Spells, "Spells" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Cells, "Cells" },
        { CSMWorld::UniversalId::Class_RecordList, CSMWorld::UniversalId::Type_Referenceables,
            "Referenceables" },

        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0 } // end marker
    };

    static const TypeData sIdArg[] =
    {
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Global, "Global Variable" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Gmst, "Game Setting" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Skill, "Skill" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Class, "Class" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Faction, "Faction" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Race, "Race" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Sound, "Sound" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Script, "Script" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Region, "Region" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Birthsign, "Birthsign" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Spell, "Spell" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Cell, "Cell" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Referenceable, "Referenceables" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Activator, "Activator" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Potion, "Potion" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Apparatus, "Apparatus" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Armor, "Armor" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Book, "Book" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Clothing, "Clothing" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Container, "Container" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Creature, "Creature" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Door, "Door" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Ingredient, "Ingredient" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_CreatureLevelledList,
            "Creature Levelled List" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_ItemLevelledList,
            "Item Levelled List" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Light, "Light" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Lockpick, "Lockpick" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Miscellaneous,
            "Miscellaneous" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Npc, "NPC" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Probe, "Probe" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Repair, "Repair" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Static, "Static" },
        { CSMWorld::UniversalId::Class_Record, CSMWorld::UniversalId::Type_Weapon, "Weapon" },

        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0 } // end marker
    };

    static const TypeData sIndexArg[] =
    {
        { CSMWorld::UniversalId::Class_Transient, CSMWorld::UniversalId::Type_VerificationResults, "Verification Results" },

        { CSMWorld::UniversalId::Class_None, CSMWorld::UniversalId::Type_None, 0 } // end marker
    };

    static const unsigned int IDARG_SIZE = sizeof (sIdArg) / sizeof (TypeData);
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


std::pair<int, const char *> CSMWorld::UniversalId::getIdArgPair (unsigned int index)
{
    std::pair<int, const char *> retPair;

    if ( index < IDARG_SIZE )
    {
        retPair.first = sIdArg[index].mType;
        retPair.second = sIdArg[index].mName;
    }

    return retPair;
}

unsigned int CSMWorld::UniversalId::getIdArgSize()
{
   return IDARG_SIZE;
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
