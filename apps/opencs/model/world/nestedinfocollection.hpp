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
            ~NestedInfoCollection() override;

            void addNestedRow(int row, int column, int position) override;

            void removeNestedRows(int row, int column, int subRow) override;

            QVariant getNestedData(int row, int column, int subRow, int subColumn) const override;

            void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn) override;

            NestedTableWrapperBase* nestedTable(int row, int column) const override;

            void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable) override;

            int getNestedRowsCount(int row, int column) const override;

            int getNestedColumnsCount(int row, int column) const override;

            // this method is inherited from NestedCollection, not from Collection<Info, IdAccessor<Info> >
            NestableColumn *getNestableColumn(int column) override;

            void addAdapter(std::pair<const ColumnBase*, NestedColumnAdapter<Info>* > adapter);
    };
}

#endif // CSM_WOLRD_NESTEDINFOCOLLECTION_H
