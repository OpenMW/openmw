#ifndef CSM_WOLRD_IDCOLLECTION_H
#define CSM_WOLRD_IDCOLLECTION_H

#include <filesystem>
#include <memory>
#include <string>

#include <apps/opencs/model/world/record.hpp>

#include <components/esm/esmcommon.hpp>
#include <components/esm3/loadland.hpp>

#include "collection.hpp"
#include "land.hpp"
#include "pathgrid.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct Pathgrid;

    /// \brief Single type collection of top level records
    template <typename ESXRecordT>
    class IdCollection : public Collection<ESXRecordT>
    {
        virtual void loadRecord(ESXRecordT& record, ESM::ESMReader& reader, bool& isDeleted);

    public:
        /// \return Index of loaded record (-1 if no record was loaded)
        int load(ESM::ESMReader& reader, bool base);

        /// \param index Index at which the record can be found.
        /// Special values: -2 index unknown, -1 record does not exist yet and therefore
        /// does not have an index
        ///
        /// \return index
        int load(const ESXRecordT& record, bool base, int index = -2);

        bool tryDelete(const ESM::RefId& id);
        ///< Try deleting \a id. If the id does not exist or can't be deleted the call is ignored.
        ///
        /// \return Has the ID been deleted?
    };

    template <typename ESXRecordT>
    void IdCollection<ESXRecordT>::loadRecord(ESXRecordT& record, ESM::ESMReader& reader, bool& isDeleted)
    {
        record.load(reader, isDeleted);
    }

    template <>
    inline void IdCollection<Land>::loadRecord(Land& record, ESM::ESMReader& reader, bool& isDeleted)
    {
        record.load(reader, isDeleted);

        // Load all land data for now. A future optimisation may only load non-base data
        // if a suitable mechanism for avoiding race conditions can be established.
        int flags = ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX;
        record.loadData(flags);

        // Prevent data from being reloaded.
        record.mContext.filename.clear();
    }

    template <typename ESXRecordT>
    int IdCollection<ESXRecordT>::load(ESM::ESMReader& reader, bool base)
    {
        ESXRecordT record;
        bool isDeleted = false;

        loadRecord(record, reader, isDeleted);
        if constexpr (std::is_same_v<ESXRecordT, LandTexture>)
        {
            // This doesn't really matter since the value never gets saved, but it makes the index uniqueness check more
            // sensible
            if (!base)
                record.mPluginIndex = -1;
        }

        ESM::RefId id = getRecordId(record);
        int index = this->searchId(id);

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

            auto baseRecord = std::make_unique<Record<ESXRecordT>>(this->getRecord(index));
            baseRecord->mState = RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return load(record, base, index);
    }

    template <typename ESXRecordT>
    int IdCollection<ESXRecordT>::load(const ESXRecordT& record, bool base, int index)
    {
        if (index == -2) // index unknown
            index = this->searchId(getRecordId(record));

        if (index == -1)
        {
            // new record
            auto record2 = std::make_unique<Record<ESXRecordT>>();
            record2->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
            (base ? record2->mBase : record2->mModified) = record;

            index = this->getSize();
            this->appendRecord(std::move(record2));
        }
        else
        {
            // old record
            auto record2 = std::make_unique<Record<ESXRecordT>>(Collection<ESXRecordT>::getRecord(index));

            if (base)
                record2->mBase = record;
            else
                record2->setModified(record);

            this->setRecord(index, std::move(record2));
        }

        return index;
    }

    template <typename ESXRecordT>
    bool IdCollection<ESXRecordT>::tryDelete(const ESM::RefId& id)
    {
        int index = this->searchId(id);

        if (index == -1)
            return false;

        const Record<ESXRecordT>& record = Collection<ESXRecordT>::getRecord(index);

        if (record.isDeleted())
            return false;

        if (record.mState == RecordBase::State_ModifiedOnly)
        {
            Collection<ESXRecordT>::removeRows(index, 1);
        }
        else
        {
            auto record2 = std::make_unique<Record<ESXRecordT>>(Collection<ESXRecordT>::getRecord(index));
            record2->mState = RecordBase::State_Deleted;
            this->setRecord(index, std::move(record2));
        }

        return true;
    }

    template <>
    int IdCollection<Pathgrid>::load(ESM::ESMReader& reader, bool base);
}

#endif
