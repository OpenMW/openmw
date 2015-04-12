#ifndef CSM_WOLRD_NESTEDCOLUMNADAPTER_H
#define CSM_WOLRD_NESTEDCOLUMNADAPTER_H

class QVariant;

namespace CSMWorld
{
    struct NestedTableWrapperBase;

    template <typename ESXRecordT>
    struct Record;

    template<typename ESXRecordT>
    class NestedColumnAdapter
    {
    public:

        NestedColumnAdapter() {}

        virtual ~NestedColumnAdapter() {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const = 0;

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const = 0;

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const = 0;

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const = 0;

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const = 0;

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value, int subRowIndex, int subColIndex) const = 0;

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const = 0;

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const = 0;
    };
}

#endif // CSM_WOLRD_NESTEDCOLUMNADAPTER_H
