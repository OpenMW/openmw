#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include "idcollection.hpp"

namespace CSMWorld
{
    template<typename ESXRecordT>
    struct FloatValueColumn : public Column<ESXRecordT>
    {
        FloatValueColumn() : Column<ESXRecordT> ("Value") {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mValue;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT base = record.getBase();
            base.mValue = data.toFloat();
            record.setModified (base);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

}

#endif