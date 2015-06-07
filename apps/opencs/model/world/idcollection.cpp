#include "idcollection.hpp"

namespace CSMWorld
{
    template<>
    int IdCollection<ESM::Script, IdAccessor<ESM::Script> >::load (ESM::ESMReader& reader, bool base)
    {
        ESM::Script record;
        std::string id = record.loadData(reader);

        if (reader.isNextSub ("DELE"))
        {
            int index = Collection<ESM::Script, IdAccessor<ESM::Script> >::searchId (id);

            //record.loadScript(reader);
            reader.skipRecord();

            if (index==-1)
            {
                // deleting a record that does not exist

                // ignore it for now

                /// \todo report the problem to the user
            }
            else if (base)
            {
                Collection<ESM::Script, IdAccessor<ESM::Script> >::removeRows (index, 1);
            }
            else
            {
                Record<ESM::Script> oldRecord = Collection<ESM::Script, IdAccessor<ESM::Script> >::getRecord (index);
                //oldRecord.setModified(record); // keep the new one, too
                oldRecord.mState = RecordBase::State_Deleted;
                this->setRecord (index, oldRecord);
            }

            return -1;
        }
        else
        {
            int index = this->searchId (id);

            if (index==-1)
                IdAccessor<ESM::Script>().getId (record) = id;

            record.loadScript(reader);

            return load (record, base, index);
        }
    }
}
