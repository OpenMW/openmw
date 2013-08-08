#ifndef CSM_WOLRD_COLUMNBASE_H
#define CSM_WOLRD_COLUMNBASE_H

#include <string>

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
            Role_Display = Qt::UserRole+1
        };

        enum Flags
        {
            Flag_Table = 1, // column should be displayed in table view
            Flag_Dialogue = 2 // column should be displayed in dialogue view
        };

        enum Display
        {
            Display_String,
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
            Display_RefRecordType
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
    };

    template<typename ESXRecordT>
    struct Column : public ColumnBase
    {
        int mFlags;

        Column (int columnId, Display displayType, int flags = Flag_Table | Flag_Dialogue)
        : ColumnBase (columnId, displayType, flags) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const = 0;

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            throw std::logic_error ("Column " + getTitle() + " is not editable");
        }
    };
}

#endif
