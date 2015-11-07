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
            virtual void loadRecord (ESXRecordT& record, ESM::ESMReader& reader);

        public:

            /// \return Index of loaded record (-1 if no record was loaded)
            int load (ESM::ESMReader& reader, bool base);

            /// \param index Index at which the record can be found.
            /// Special values: -2 index unknown, -1 record does not exist yet and therefore
            /// does not have an index
            ///
            /// \return index
            int load (const ESXRecordT& record, bool base, int index = -2);

            bool tryDelete (const std::string& id);
            ///< Try deleting \a id. If the id does not exist or can't be deleted the call is ignored.
            ///
            /// \return Has the ID been deleted?
    };

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::loadRecord (ESXRecordT& record,
        ESM::ESMReader& reader)
    {
        record.load (reader);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::load (ESM::ESMReader& reader, bool base)
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

            return -1;
        }
        else
        {
            ESXRecordT record;

            // Sometimes id (i.e. NAME of the cell) may be different to the id we stored
            // earlier.  e.g. NAME == "Vivec, Arena" but id == "#-4 11".  Sometime NAME is
            // missing altogether for scripts or cells.
            //
            // In such cases the returned index will be -1.  We then try updating the
            // IdAccessor's id manually (e.g. set mId of the record to "Vivec, Arena")
            // and try getting the index once more after loading the record.  The mId of the
            // record would have changed to "#-4 11" after the load, and searchId() should find
            // it (if this is a modify)
            int index = this->searchId (id);

            if (index==-1)
                IdAccessorT().getId (record) = id;
            else
            {
                record = this->getRecord (index).get();
            }

            loadRecord (record, reader);

            if (index==-1)
            {
                std::string newId = IdAccessorT().getId(record);
                int newIndex = this->searchId(newId);
                if (newIndex != -1 && id != newId)
                    index = newIndex;
            }

            return load (record, base, index);
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::load (const ESXRecordT& record, bool base,
        int index)
    {
        if (index==-2)
            index = this->searchId (IdAccessorT().getId (record));

        if (index==-1)
        {
            // new record
            Record<ESXRecordT> record2;
            record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record2.mBase : record2.mModified) = record;

            index = this->getSize();
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

        return index;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    bool IdCollection<ESXRecordT, IdAccessorT>::tryDelete (const std::string& id)
    {
        int index = this->searchId (id);

        if (index==-1)
            return false;

        Record<ESXRecordT> record = Collection<ESXRecordT, IdAccessorT>::getRecord (index);

        if (record.isDeleted())
            return false;

        if (record.mState==RecordBase::State_ModifiedOnly)
        {
            Collection<ESXRecordT, IdAccessorT>::removeRows (index, 1);
        }
        else
        {
            record.mState = RecordBase::State_Deleted;
            this->setRecord (index, record);
        }

        return true;
    }
}

#endif
