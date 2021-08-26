#include "idcollection.hpp"

namespace CSMWorld
{
    template<>
    int IdCollection<Pathgrid, IdAccessor<Pathgrid> >::load (ESM::ESMReader& reader, bool base)
    {
        Pathgrid record;
        bool isDeleted = false;

        loadRecord (record, reader, isDeleted);

        std::string id = IdAccessor<Pathgrid>().getId (record);
        int index = this->searchId (id);

        if (record.mPoints.empty() || record.mEdges.empty())
            isDeleted = true;

        if (isDeleted)
        {
            if (index==-1)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            if (base)
            {
                this->removeRows (index, 1);
                return -1;
            }

            std::unique_ptr<Record<Pathgrid> > baseRecord(new Record<Pathgrid>(this->getRecord(index)));
            baseRecord->mState = RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return load (record, base, index);
    }
}
