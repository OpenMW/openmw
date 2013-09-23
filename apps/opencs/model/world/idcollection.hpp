#ifndef CSM_WOLRD_IDCOLLECTION_H
#define CSM_WOLRD_IDCOLLECTION_H

#include <components/esm/esmreader.hpp>

#include "collection.hpp"

namespace CSMWorld
{

    /// \brief Single type collection of top level records
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class IdCollection : public Collection<ESXRecordT, IdAccessorT>
    {
        public:

            void load (ESM::ESMReader& reader, bool base,
                UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types
    };

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::load (ESM::ESMReader& reader, bool base,
        UniversalId::Type type)
    {
        std::string id = reader.getHNOString ("NAME");

        if (reader.isNextSub ("DELE"))
        {
            int index = Collection<ESXRecordT, IdAccessorT>::searchId (id);

            reader.skipRecord();

            if (index==-1)
            {
                // deleting a record that does not exist

                // ignore it for now

                /// \todo report the problem to the user
            }
            else if (base)
            {
                Collection<ESXRecordT, IdAccessorT>::removeRows (index, 1);
            }
            else
            {
                Record<ESXRecordT> record = Collection<ESXRecordT, IdAccessorT>::getRecord (index);
                record.mState = RecordBase::State_Deleted;
                this->setRecord (index, record);
            }
        }
        else
        {
            ESXRecordT record;
            IdAccessorT().getId (record) = id;
            record.load (reader);

            int index = this->searchId (IdAccessorT().getId (record));

            if (index==-1)
            {
                // new record
                Record<ESXRecordT> record2;
                record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
                (base ? record2.mBase : record2.mModified) = record;

                this->appendRecord (record2);
            }
            else
            {
                // old record
                Record<ESXRecordT> record2 = Collection<ESXRecordT, IdAccessorT>::getRecord (index);

                if (base)
                    record2.mBase = record;
                else
                    record2.setModified (record);

                this->setRecord (index, record2);
            }
        }
    }
}

#endif
