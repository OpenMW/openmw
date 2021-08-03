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
        enum TableEditModes
        {
            TableEdit_None,      // no editing
            TableEdit_Full,      // edit cells and add/remove rows
            TableEdit_FixedRows  // edit cells only
        };

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
            Flag_Dialogue_List = 4, // column should be diaplyed in dialogue view
            Flag_Dialogue_Refresh = 8 // refresh dialogue view if this column is modified
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
            //CONCRETE TYPES ENDS HERE

            Display_SignedInteger8,
            Display_SignedInteger16,
            Display_UnsignedInteger8,
            Display_UnsignedInteger16,
            Display_Integer,
            Display_Float,
            Display_Double,
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
            Display_SkillId,
            Display_EffectRange,
            Display_EffectId,
            Display_PartRefType,
            Display_AiPackageType,
            Display_InfoCondFunc,
            Display_InfoCondVar,
            Display_InfoCondComp,
            Display_String32,
            Display_String64,
            Display_LongString256,
            Display_BookType,
            Display_BloodType,
            Display_EmitterType,

            Display_EffectSkill,     // must display at least one, unlike Display_Skill
            Display_EffectAttribute, // must display at least one, unlike Display_Attribute
            Display_IngredEffectId,  // display none allowed, unlike Display_EffectId
            Display_GenderNpc,       // must display at least one, unlike Display_Gender

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
        NestedParentColumn (int id, int flags = ColumnBase::Flag_Dialogue, bool fixedRows = false)
            : Column<ESXRecordT> (id, ColumnBase::Display_NestedHeader, flags), mFixedRows(fixedRows)
        {}

        void set (Record<ESXRecordT>& record, const QVariant& data) override
        {
            // There is nothing to do here.
            // This prevents exceptions from parent's implementation
        }

        QVariant get (const Record<ESXRecordT>& record) const override
        {
            // by default editable; also see IdTree::hasChildren()
            if (mFixedRows)
                return QVariant::fromValue(ColumnBase::TableEdit_FixedRows);
            else
                return QVariant::fromValue(ColumnBase::TableEdit_Full);
        }

        bool isEditable() const override
        {
            return true;
        }

    private:
        bool mFixedRows;
    };

    struct NestedChildColumn : public NestableColumn
    {
        NestedChildColumn (int id,
                Display display, int flags = ColumnBase::Flag_Dialogue, bool isEditable = true);

        bool isEditable() const override;

    private:
        bool mIsEditable;
    };
}

Q_DECLARE_METATYPE(CSMWorld::ColumnBase::TableEditModes)

#endif
