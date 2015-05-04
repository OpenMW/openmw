#ifndef CSM_WOLRD_COLUMNBASE_H
#define CSM_WOLRD_COLUMNBASE_H

#include <string>
#include <vector>
#include <stdexcept>

#include <Qt>
#include <QVariant>

#include "record.hpp"

namespace CSMWorld
{
    struct ColumnBase
    {
        enum Roles
        {
            Role_Flags = Qt::UserRole,
            Role_Display = Qt::UserRole+1,
            Role_ColumnId = Qt::UserRole+2
        };

        enum Flags
        {
            Flag_Table = 1, // column should be displayed in table view
            Flag_Dialogue = 2, // column should be displayed in dialogue view
            Flag_Dialogue_List = 4 // column should be diaplyed in dialogue view
        };

        enum Display
        {
            Display_None, //Do not use
            Display_String,
            Display_LongString,

            //CONCRETE TYPES STARTS HERE (for drag and drop)
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
            //CONCRETE TYPES ENDS HERE

            Display_Integer,
            Display_Float,
            Display_Var,
            Display_GmstVarType,
            Display_GlobalVarType,
            Display_Specialisation,
            Display_Attribute,
            Display_Boolean,
            Display_SpellType,
            Display_Script,
            Display_ApparatusType,
            Display_ArmorType,
            Display_ClothingType,
            Display_CreatureType,
            Display_WeaponType,
            Display_RecordState,
            Display_RefRecordType,
            Display_DialogueType,
            Display_QuestStatusType,
            Display_EnchantmentType,
            Display_BodyPartType,
            Display_MeshType,
            Display_Gender,
            Display_Mesh,
            Display_Icon,
            Display_Music,
            Display_SoundRes,
            Display_Texture,
            Display_Video,
            Display_Colour,
            Display_ScriptFile,
            Display_ScriptLines, // console context
            Display_SoundGeneratorType,
            Display_School,
            Display_Id,
            Display_SkillImpact,
            Display_EffectRange,
            Display_EffectId,
            Display_PartRefType,
            Display_AiPackageType,
            Display_YesNo,

            //top level columns that nest other columns
            Display_NestedHeader
        };

        int mColumnId;
        int mFlags;
        Display mDisplayType;

        ColumnBase (int columnId, Display displayType, int flag);

        virtual ~ColumnBase();

        virtual bool isEditable() const = 0;

        virtual bool isUserEditable() const;
        ///< Can this column be edited directly by the user?

        virtual std::string getTitle() const;

        virtual int getId() const;

        static bool isId (Display display);

        static bool isText (Display display);

        static bool isScript (Display display);
    };

    class NestableColumn : public ColumnBase
    {
        std::vector<NestableColumn *> mNestedColumns;

    public:

        NestableColumn(int columnId, Display displayType, int flag);

        ~NestableColumn();

        void addColumn(CSMWorld::NestableColumn *column);

        const ColumnBase& nestedColumn(int subColumn) const;

        bool hasChildren() const;
    };

    template<typename ESXRecordT>
    struct Column : public NestableColumn
    {
        Column (int columnId, Display displayType, int flags = Flag_Table | Flag_Dialogue)
        : NestableColumn (columnId, displayType, flags) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const = 0;

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            throw std::logic_error ("Column " + getTitle() + " is not editable");
        }
    };

    template<typename ESXRecordT>
    struct NestedParentColumn : public Column<ESXRecordT>
    {
        NestedParentColumn (int id, int flags = ColumnBase::Flag_Dialogue) : Column<ESXRecordT> (id,
                ColumnBase::Display_NestedHeader, flags)
        {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return true; // required by IdTree::hasChildren()
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    struct NestedChildColumn : public NestableColumn
    {
        NestedChildColumn (int id, Display display, bool isEditable = true);

        virtual bool isEditable() const;

    private:
        bool mIsEditable;
    };
}

#endif
