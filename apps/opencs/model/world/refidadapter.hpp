#ifndef CSM_WOLRD_REFIDADAPTER_H
#define CSM_WOLRD_REFIDADAPTER_H

#include <string>
#include <vector>

/*! \brief
 * Adapters acts as indirection layer, abstracting details of the record types (in the wrappers) from the higher levels of model.
 * Please notice that nested adaptor uses helper classes for actually performing any actions. Different record types require different helpers (needs to be created in the subclass and then fetched via member function).
 *
 * Important point: don't forget to make sure that getData on the nestedColumn returns true (otherwise code will not treat the index pointing to the column as having children!
 */

class QVariant;

namespace CSMWorld
{
    class RefIdColumn;
    class RefIdData;
    struct RecordBase;
    struct NestedTableWrapperBase;
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

            virtual void setId(RecordBase& record, const std::string& id) = 0; // used by RefIdCollection::cloneRecord()
    };

    class NestedRefIdAdapterBase
    {
        public:
            NestedRefIdAdapterBase();

            virtual ~NestedRefIdAdapterBase();

            virtual void setNestedData (const RefIdColumn *column,
                    RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const = 0;

            virtual QVariant getNestedData (const RefIdColumn *column,
                    const RefIdData& data, int index, int subRowIndex, int subColIndex) const = 0;

            virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const = 0;

            virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const = 0;

            virtual void removeNestedRow (const RefIdColumn *column,
                    RefIdData& data, int index, int rowToRemove) const = 0;

            virtual void addNestedRow (const RefIdColumn *column,
                    RefIdData& data, int index, int position) const = 0;

            virtual void setNestedTable (const RefIdColumn* column,
                    RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const = 0;

            virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                    const RefIdData& data, int index) const = 0;
    };
}

#endif
