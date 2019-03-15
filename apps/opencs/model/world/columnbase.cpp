#include "columnbase.hpp"

#include "columns.hpp"

CSMWorld::ColumnBase::ColumnBase (int columnId, Display displayType, int flags)
    : mColumnId (columnId), mFlags (flags), mDisplayType (displayType)
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

int  CSMWorld::ColumnBase::getId() const
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
        Display_Rank,
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
        Display_BodyPart,
        Display_Enchantment,
        Display_Script,

        Display_Mesh,
        Display_Icon,
        Display_Music,
        Display_SoundRes,
        Display_Texture,
        Display_Video,

        Display_Id,
        Display_SkillId,
        Display_EffectRange,
        Display_EffectId,
        Display_PartRefType,
        Display_AiPackageType,
        Display_InfoCondFunc,
        Display_InfoCondVar,
        Display_InfoCondComp,

        Display_EffectSkill,
        Display_EffectAttribute,
        Display_IngredEffectId,

        Display_None
    };

    for (int i=0; ids[i]!=Display_None; ++i)
        if (ids[i]==display)
            return true;

    return false;
}

bool CSMWorld::ColumnBase::isText (Display display)
{
    return display==Display_String || display==Display_LongString ||
        display==Display_String32 || display==Display_LongString256;
}

bool CSMWorld::ColumnBase::isScript (Display display)
{
    return display==Display_ScriptFile || display==Display_ScriptLines;
}

void CSMWorld::NestableColumn::addColumn(CSMWorld::NestableColumn *column)
{
    mNestedColumns.push_back(column);
}

const CSMWorld::ColumnBase& CSMWorld::NestableColumn::nestedColumn(int subColumn) const
{
    if (mNestedColumns.empty())
        throw std::logic_error("Tried to access nested column of the non-nest column");

    return *mNestedColumns.at(subColumn);
}

CSMWorld::NestableColumn::NestableColumn(int columnId, CSMWorld::ColumnBase::Display displayType,
    int flag)
    : CSMWorld::ColumnBase(columnId, displayType, flag)
{}

CSMWorld::NestableColumn::~NestableColumn()
{
    for (unsigned int i = 0; i < mNestedColumns.size(); ++i)
    {
        delete mNestedColumns[i];
    }
}

bool CSMWorld::NestableColumn::hasChildren() const
{
    return !mNestedColumns.empty();
}

CSMWorld::NestedChildColumn::NestedChildColumn (int id,
    CSMWorld::ColumnBase::Display display, int flags, bool isEditable)
    : NestableColumn (id, display, flags) , mIsEditable(isEditable)
{}

bool CSMWorld::NestedChildColumn::isEditable () const
{
    return mIsEditable;
}
