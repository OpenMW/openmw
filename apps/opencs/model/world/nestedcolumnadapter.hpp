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

        virtual void addRow(Record<ESXRecordT>& record, int position) const = 0;

        virtual void removeRow(Record<ESXRecordT>& record, int rowToRemove) const = 0;

        virtual void setTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const = 0;

        virtual NestedTableWrapperBase* table(const Record<ESXRecordT>& record) const = 0;

        virtual QVariant getData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const = 0;

        virtual void setData(Record<ESXRecordT>& record, const QVariant& value, int subRowIndex, int subColIndex) const = 0;

        virtual int getColumnsCount(const Record<ESXRecordT>& record) const = 0;

        virtual int getRowsCount(const Record<ESXRecordT>& record) const = 0;
    };
}

#endif // CSM_WOLRD_NESTEDCOLUMNADAPTER_H
