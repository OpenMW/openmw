#ifndef CSM_WOLRD_REFIDADAPTER_H
#define CSM_WOLRD_REFIDADAPTER_H

#include <string>

class QVariant;

namespace CSMWorld
{
    class RefIdColumn;
    class RefIdData;
    class RecordBase;

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

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const = 0;
            ///< If the data type does not match an exception is thrown.

            virtual std::string getId (const RecordBase& record) const = 0;

            virtual void setId(RecordBase& record, const std::string& id) = 0;
    };

    class NestedRefIdAdapter
    {
        public:
            NestedRefIdAdapter();

            virtual ~NestedRefIdAdapter();
 
            virtual void setNestedData (const RefIdColumn *column, RefIdData& data, int row,
					const QVariant& value, int subRowIndex, int subColIndex) const = 0;

	    virtual QVariant getNestedData (const RefIdColumn *column, const RefIdData& data,
					    int index, int subRowIndex, int subColIndex) const = 0;
      
      
	    virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const = 0;

	    virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const = 0;
    };
}

#endif
