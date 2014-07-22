#ifndef CSM_WOLRD_REFIDADAPTER_H
#define CSM_WOLRD_REFIDADAPTER_H

#include <string>
#include <vector>

#include "nestedadaptors.hpp"

class QVariant;

namespace CSMWorld
{
    class RefIdColumn;
    class RefIdData;
    class RecordBase;
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
        
    private:
        
        HelperBase* getHelper(const RefIdColumn *column) const;
    };
}

#endif
