#ifndef CSM_WOLRD_NESTEDINFOCOLLECTION_H
#define CSM_WOLRD_NESTEDINFOCOLLECTION_H

#include <map>

#include "infocollection.hpp"
#include "nestedcollection.hpp"

namespace CSMWorld
{
    struct NestedTableWrapperBase;

    template<typename ESXRecordT>
    class NestedColumnAdapter;

    class NestedInfoCollection : public InfoCollection, public NestedCollection
    {
            std::map<const ColumnBase*, NestedColumnAdapter<Info>* > mAdapters;

            const NestedColumnAdapter<Info>& getAdapter(const ColumnBase &column) const;

        public:

            NestedInfoCollection ();
            ~NestedInfoCollection();

            virtual void addNestedRow(int row, int column, int position);

            virtual void removeNestedRows(int row, int column, int subRow);

            virtual QVariant getNestedData(int row, int column, int subRow, int subColumn) const;

            virtual void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn);

            virtual NestedTableWrapperBase* nestedTable(int row, int column) const;

            virtual void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable);

            virtual int getNestedRowsCount(int row, int column) const;

            virtual int getNestedColumnsCount(int row, int column) const;

            // this method is inherited from NestedCollection, not from Collection<Info, IdAccessor<Info> >
            virtual NestableColumn *getNestableColumn(int column);

            void addAdapter(std::pair<const ColumnBase*, NestedColumnAdapter<Info>* > adapter);
    };
}

#endif // CSM_WOLRD_NESTEDINFOCOLLECTION_H
