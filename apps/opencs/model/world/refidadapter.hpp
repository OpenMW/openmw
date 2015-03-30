#ifndef CSM_WOLRD_REFIDADAPTER_H
#define CSM_WOLRD_REFIDADAPTER_H

#include <string>
#include <vector>

#include "nestedadapters.hpp"

/*! \brief
 * Adapters acts as indirection layer, abstracting details of the record types (in the wrappers) from the higher levels of model.
 * Please notice that nested adaptor uses helper classes for actually performing any actions. Different record types require different helpers (needs to be created in the subclass and then fetched via member function).
 * Important point: don't forget to make sure that getData on the nestedColumn returns true (otherwise code will not treat the index pointing to the column as having childs!
 */

class QVariant;

namespace CSMWorld
{
    class RefIdColumn;
    class RefIdData;
    struct RecordBase;
    class NestedTableWrapperBase;
    class HelperBase;

    class RefIdAdapter
    {
            // not implemented
            RefIdAdapter (const RefIdAdapter&);
            RefIdAdapter& operator= (const RefIdAdapter&);

        public:

            RefIdAdapter();

            virtual ~RefIdAdapter();

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int idnex)
                const = 0;
            ///< If called on the nest column, should return QVariant(true).

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const = 0;
            ///< If the data type does not match an exception is thrown.

            virtual std::string getId (const RecordBase& record) const = 0;

            virtual void setId(RecordBase& record, const std::string& id) = 0;
    };

    class NestedRefIdAdapterBase
    {
        public:
            NestedRefIdAdapterBase();

            virtual ~NestedRefIdAdapterBase();

            virtual void setNestedData (const RefIdColumn *column, RefIdData& data, int row,
                                        const QVariant& value, int subRowIndex, int subColIndex) const = 0;

            virtual QVariant getNestedData (const RefIdColumn *column, const RefIdData& data,
                                            int index, int subRowIndex, int subColIndex) const = 0;

            virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const = 0;

            virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const = 0;

            virtual void removeNestedRow (const RefIdColumn *column, RefIdData& data, int index, int rowToRemove) const = 0;

            virtual void addNestedRow (const RefIdColumn *column, RefIdData& data, int index, int position) const = 0;

            virtual void setNestedTable (const RefIdColumn* column, RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) = 0;

            virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column, const RefIdData& data, int index) const = 0;
    };

    class NestedRefIdAdapter : public NestedRefIdAdapterBase
    {
        std::vector<std::pair <const RefIdColumn*, HelperBase*> >  mAssociatedColumns; //basicly, i wanted map, but with pointer key

    public:
        NestedRefIdAdapter();

        virtual ~NestedRefIdAdapter();

        virtual void setNestedData (const RefIdColumn *column, RefIdData& data, int row,
                                    const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual QVariant getNestedData (const RefIdColumn *column, const RefIdData& data,
                                        int index, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;

        virtual void removeNestedRow (const RefIdColumn *column, RefIdData& data, int index, int rowToRemove) const;

        virtual void addNestedRow (const RefIdColumn *column, RefIdData& data, int index, int position) const;

        virtual void setNestedTable (const RefIdColumn* column, RefIdData& data, int index, const NestedTableWrapperBase& nestedTable);

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column, const RefIdData& data, int index) const;

    protected:
        void setAssocColumns(const std::vector<std::pair <const RefIdColumn*, HelperBase*> >& assocColumns);
        ///The ownership of the Helper pointers is transfered.
        ///The ownership of the column pointers it not transfered (it is not surprising, since columns are created by collection).
        ///You MUST call this method to setup the nested adaptor!

        void addAssocColumn(const std::pair <const RefIdColumn*, HelperBase*>& assocColumn);
        ///Like setAssocColumn, when it is impossible to set all columns at once

    private:

        HelperBase* getHelper(const RefIdColumn *column) const;
    };
}

#endif
