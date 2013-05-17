#ifndef CSM_WOLRD_REFIDCOLLECTION_H
#define CSM_WOLRD_REFIDCOLLECTION_H

#include <vector>
#include <map>
#include <deque>

#include "columnbase.hpp"
#include "idcollection.hpp"
#include "refiddata.hpp"

namespace CSMWorld
{
    class RefIdAdapter;

    class RefIdColumn : public ColumnBase
    {
            bool mEditable;
            bool mUserEditable;

        public:

            RefIdColumn (const std::string& title, Display displayType,
                int flag = Flag_Table | Flag_Dialogue, bool editable = true,
                bool userEditable = true);

            virtual bool isEditable() const;

            virtual bool isUserEditable() const;
    };

    class RefIdCollection : public IdCollectionBase
    {
        private:

            RefIdData mData;
            std::deque<RefIdColumn> mColumns;
            std::map<UniversalId::Type, RefIdAdapter *> mAdapters;

        private:

            const RefIdAdapter& findAdaptor (UniversalId::Type) const;
            ///< Throws an exception if no adaptor for \a Type can be found.

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

            virtual void load (ESM::ESMReader& reader, bool base, UniversalId::Type type);

            virtual int getAppendIndex (UniversalId::Type type) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types
    };
}

#endif
