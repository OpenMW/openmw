
#include "columnbase.hpp"

#include "columns.hpp"

CSMWorld::ColumnBase::ColumnBase (int columnId, Display displayType, int flags)
: mColumnId (columnId), mDisplayType (displayType), mFlags (flags)
{}

CSMWorld::ColumnBase::~ColumnBase() {}

bool CSMWorld::ColumnBase::isUserEditable() const
{
    return isEditable();
}

std::string CSMWorld::ColumnBase::getTitle() const
{
    return Columns::getName (static_cast<Columns::ColumnId> (mColumnId));
}

int CSMWorld::ColumnBase::getId() const
{
    return mColumnId;
}

bool CSMWorld::ColumnBase::isId (Display display)
{
    static const Display ids[] =
    {
        Display_Skill,
        Display_Class,
        Display_Faction,
        Display_Race,
        Display_Sound,
        Display_Region,
        Display_Birthsign,
        Display_Spell,
        Display_Cell,
        Display_Referenceable,
        Display_Activator,
        Display_Potion,
        Display_Apparatus,
        Display_Armor,
        Display_Book,
        Display_Clothing,
        Display_Container,
        Display_Creature,
        Display_Door,
        Display_Ingredient,
        Display_CreatureLevelledList,
        Display_ItemLevelledList,
        Display_Light,
        Display_Lockpick,
        Display_Miscellaneous,
        Display_Npc,
        Display_Probe,
        Display_Repair,
        Display_Static,
        Display_Weapon,
        Display_Reference,
        Display_Filter,
        Display_Topic,
        Display_Journal,
        Display_TopicInfo,
        Display_JournalInfo,
        Display_Scene,
        Display_GlobalVariable,

        Display_Mesh,
        Display_Icon,
        Display_Music,
        Display_SoundRes,
        Display_Texture,
        Display_Video,
        
        Display_None
    };

    for (int i=0; ids[i]!=Display_None; ++i)
        if (ids[i]==display)
            return true;

    return false;    
}

bool CSMWorld::ColumnBase::isText (Display display)
{
    return display==Display_String || display==Display_LongString;
}

bool CSMWorld::ColumnBase::isScript (Display display)
{
    return display==Display_Script || display==Display_ScriptLines;
}
