#include "idcollection.hpp"

#include <memory>
#include <string>
#include <utility>

#include <apps/opencs/model/world/collection.hpp>
#include <apps/opencs/model/world/pathgrid.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/loadpgrd.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    template <>
    int IdCollection<Pathgrid>::load(ESM::ESMReader& reader, bool base)
    {
        Pathgrid record;
        bool isDeleted = false;

        loadRecord(record, reader, isDeleted);

        const ESM::RefId id = getRecordId(record);
        int index = this->searchId(id);

        if (record.mPoints.empty() || record.mEdges.empty())
            isDeleted = true;

        if (isDeleted)
        {
            if (index == -1)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            if (base)
            {
                this->removeRows(index, 1);
                return -1;
            }

            auto baseRecord = std::make_unique<Record<Pathgrid>>(this->getRecord(index));
            baseRecord->mState = RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return load(record, base, index);
    }
}
