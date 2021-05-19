#ifndef CSM_WOLRD_REFIDCOLLECTION_H
#define CSM_WOLRD_REFIDCOLLECTION_H

#include <vector>
#include <map>
#include <deque>

#include "columnbase.hpp"
#include "collectionbase.hpp"
#include "nestedcollection.hpp"
#include "refiddata.hpp"

namespace ESM
{
    class ESMWriter;
}

namespace CSMWorld
{
    class RefIdAdapter;
    struct NestedTableWrapperBase;
    class NestedRefIdAdapterBase;

    class RefIdColumn : public NestableColumn
    {
            bool mEditable;
            bool mUserEditable;

        public:

            RefIdColumn (int columnId, Display displayType,
                         int flag = Flag_Table | Flag_Dialogue, bool editable = true,
                         bool userEditable = true);

            bool isEditable() const override;

            bool isUserEditable() const override;
    };

    class RefIdCollection : public CollectionBase, public NestedCollection
    {
        private:

            RefIdData mData;
            std::deque<RefIdColumn> mColumns;
            std::map<UniversalId::Type, RefIdAdapter *> mAdapters;

            std::vector<std::pair<const ColumnBase*, std::map<UniversalId::Type, NestedRefIdAdapterBase*> > > mNestedAdapters;

        private:

            const RefIdAdapter& findAdapter (UniversalId::Type) const;
            ///< Throws an exception if no adaptor for \a Type can be found.

            const NestedRefIdAdapterBase& getNestedAdapter(const ColumnBase &column, UniversalId::Type type) const;

        public:

            RefIdCollection();

            ~RefIdCollection() override;

            int getSize() const override;

            std::string getId (int index) const override;

            int getIndex (const std::string& id) const override;

            int getColumns() const override;

            const ColumnBase& getColumn (int column) const override;

            QVariant getData (int index, int column) const override;

            void setData (int index, int column, const QVariant& data) override;

            void removeRows (int index, int count) override;

            void cloneRecord(const std::string& origin,
                                     const std::string& destination,
                                     const UniversalId::Type type) override;

            bool touchRecord(const std::string& id) override;

            void appendBlankRecord (const std::string& id, UniversalId::Type type) override;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            int searchId (const std::string& id) const override;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            void replace (int index, const RecordBase& record) override;
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            void appendRecord (const RecordBase& record, UniversalId::Type type) override;
            ///< If the record type does not match, an exception is thrown.
            ///
            ///< \param type Will be ignored, unless the collection supports multiple record types

            const RecordBase& getRecord (const std::string& id) const override;

            const RecordBase& getRecord (int index) const override;

            void load (ESM::ESMReader& reader, bool base, UniversalId::Type type);

            int getAppendIndex (const std::string& id, UniversalId::Type type) const override;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            std::vector<std::string> getIds (bool listDeleted) const override;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            bool reorderRows (int baseIndex, const std::vector<int>& newOrder) override;
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            QVariant getNestedData(int row, int column, int subRow, int subColumn) const override;

            NestedTableWrapperBase* nestedTable(int row, int column) const override;

            void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable) override;

            int getNestedRowsCount(int row, int column) const override;

            int getNestedColumnsCount(int row, int column) const override;

            NestableColumn *getNestableColumn(int column) override;

            void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn) override;

            void removeNestedRows(int row, int column, int subRow) override;

            void addNestedRow(int row, int col, int position) override;

            void save (int index, ESM::ESMWriter& writer) const;

            const RefIdData& getDataSet() const; //I can't figure out a better name for this one :(
            void copyTo (int index, RefIdCollection& target) const;
    };
}

#endif
