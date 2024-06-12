#include "idcollection.hpp"

#include <memory>
#include <string>
#include <utility>

#include <apps/opencs/model/world/collection.hpp>
#include <apps/opencs/model/world/pathgrid.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadpgrd.hpp>

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    template <>
    int BaseIdCollection<Pathgrid>::load(ESM::ESMReader& reader, bool base)
    {
        Pathgrid record;
        bool isDeleted = false;

        loadRecord(record, reader, isDeleted, base);

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

    const Record<ESM::LandTexture>* IdCollection<ESM::LandTexture>::searchRecord(std::uint16_t index, int plugin) const
    {
        auto found = mIndices.find({ plugin, index });
        if (found != mIndices.end())
        {
            int index = searchId(found->second);
            if (index != -1)
                return &getRecord(index);
        }
        return nullptr;
    }

    const std::string* IdCollection<ESM::LandTexture>::getLandTexture(std::uint16_t index, int plugin) const
    {
        const Record<ESM::LandTexture>* record = searchRecord(index, plugin);
        if (record && !record->isDeleted())
            return &record->get().mTexture;
        return nullptr;
    }

    void IdCollection<ESM::LandTexture>::loadRecord(
        ESM::LandTexture& record, ESM::ESMReader& reader, bool& isDeleted, bool base)
    {
        record.load(reader, isDeleted);
        int plugin = base ? reader.getIndex() : -1;
        mIndices.emplace(std::make_pair(plugin, record.mIndex), record.mId);
    }

    std::uint16_t IdCollection<ESM::LandTexture>::assignNewIndex(ESM::RefId id)
    {
        std::uint16_t index = 0;
        if (!mIndices.empty())
        {
            auto end = mIndices.lower_bound({ -1, std::numeric_limits<std::uint16_t>::max() });
            if (end != mIndices.begin())
                end = std::prev(end);
            if (end->first.first == -1)
            {
                constexpr std::uint16_t maxIndex = std::numeric_limits<std::uint16_t>::max() - 1;
                if (end->first.second < maxIndex)
                    index = end->first.second + 1;
                else
                {
                    std::uint16_t prevIndex = 0;
                    for (auto it = mIndices.lower_bound({ -1, 0 }); it != end; ++it)
                    {
                        if (prevIndex != it->first.second)
                        {
                            index = prevIndex;
                            break;
                        }
                        ++prevIndex;
                    }
                }
            }
        }
        mIndices.emplace(std::make_pair(-1, index), id);
        return index;
    }

    bool IdCollection<ESM::LandTexture>::touchRecord(const ESM::RefId& id)
    {
        int row = BaseIdCollection<ESM::LandTexture>::touchRecordImp(id);
        if (row != -1)
        {
            const_cast<ESM::LandTexture&>(getRecord(row).get()).mIndex = assignNewIndex(id);
            return true;
        }
        return false;
    }

    void IdCollection<ESM::LandTexture>::cloneRecord(
        const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type)
    {
        int row = cloneRecordImp(origin, destination, type);
        const_cast<ESM::LandTexture&>(getRecord(row).get()).mIndex = assignNewIndex(destination);
    }

    void IdCollection<ESM::LandTexture>::appendBlankRecord(const ESM::RefId& id, UniversalId::Type type)
    {
        ESM::LandTexture record;
        record.blank();
        record.mId = id;
        record.mIndex = assignNewIndex(id);

        auto record2 = std::make_unique<Record<ESM::LandTexture>>();
        record2->mState = Record<ESM::LandTexture>::State_ModifiedOnly;
        record2->mModified = std::move(record);

        insertRecord(std::move(record2), getAppendIndex(id, type), type);
    }

    void IdCollection<ESM::LandTexture>::removeRows(int index, int count)
    {
        for (int row = index; row < index + count; ++row)
        {
            const auto& record = getRecord(row);
            if (record.isModified())
                mIndices.erase({ -1, record.get().mIndex });
        }
        BaseIdCollection<ESM::LandTexture>::removeRows(index, count);
    }

    void IdCollection<ESM::LandTexture>::replace(int index, std::unique_ptr<RecordBase> record)
    {
        const auto& current = getRecord(index);
        if (current.isModified() && !record->isModified())
            mIndices.erase({ -1, current.get().mIndex });
        BaseIdCollection<ESM::LandTexture>::replace(index, std::move(record));
    }
}
