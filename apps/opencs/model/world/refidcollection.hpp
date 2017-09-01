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

            virtual bool isEditable() const;

            virtual bool isUserEditable() const;
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

            virtual ~RefIdCollection();

            virtual int getSize() const;

            virtual std::string getId (int index) const;

            virtual int getIndex (const std::string& id) const;

            virtual int getColumns() const;

            virtual const ColumnBase& getColumn (int column) const;

            virtual QVariant getData (int index, int column) const;

            virtual void setData (int index, int column, const QVariant& data);

            virtual void removeRows (int index, int count);

            virtual void cloneRecord(const std::string& origin,
                                     const std::string& destination,
                                     const UniversalId::Type type);

            virtual bool touchRecord(const std::string& id);

            virtual void appendBlankRecord (const std::string& id, UniversalId::Type type);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual int searchId (const std::string& id) const;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record);
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            virtual void appendRecord (const RecordBase& record, UniversalId::Type type);
            ///< If the record type does not match, an exception is thrown.
            ///
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual const RecordBase& getRecord (const std::string& id) const;

            virtual const RecordBase& getRecord (int index) const;

            void load (ESM::ESMReader& reader, bool base, UniversalId::Type type);

            virtual int getAppendIndex (const std::string& id, UniversalId::Type type) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual std::vector<std::string> getIds (bool listDeleted) const;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            virtual bool reorderRows (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            virtual QVariant getNestedData(int row, int column, int subRow, int subColumn) const;

            virtual NestedTableWrapperBase* nestedTable(int row, int column) const;

            virtual void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable);

            virtual int getNestedRowsCount(int row, int column) const;

            virtual int getNestedColumnsCount(int row, int column) const;

            NestableColumn *getNestableColumn(int column);

            virtual void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn);

            virtual void removeNestedRows(int row, int column, int subRow);

            virtual void addNestedRow(int row, int col, int position);

            void save (int index, ESM::ESMWriter& writer) const;

            const RefIdData& getDataSet() const; //I can't figure out a better name for this one :(
            void copyTo (int index, RefIdCollection& target) const;
    };
}

#endif
