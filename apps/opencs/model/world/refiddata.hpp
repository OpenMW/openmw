#ifndef CSM_WOLRD_REFIDDATA_H
#define CSM_WOLRD_REFIDDATA_H

#include <vector>
#include <map>
#include <memory>
#include <cassert>
#include <string_view>

#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/misc/stringops.hpp>

#include "record.hpp"
#include "universalid.hpp"

namespace ESM
{
    class ESMReader;
}

namespace CSMWorld
{
    struct RefIdDataContainerBase
    {
        virtual ~RefIdDataContainerBase();

        virtual int getSize() const = 0;

        virtual const RecordBase& getRecord (int index) const = 0;

        virtual RecordBase& getRecord (int index)= 0;

        virtual unsigned int getRecordFlags (int index) const = 0;

        virtual void appendRecord (const std::string& id, bool base) = 0;

        virtual void insertRecord (std::unique_ptr<RecordBase> record) = 0;

        virtual int load (ESM::ESMReader& reader, bool base) = 0;
        ///< \return index of a loaded record or -1 if no record was loaded

        virtual void erase (int index, int count) = 0;

        virtual std::string getId (int index) const = 0;

        virtual void save (int index, ESM::ESMWriter& writer) const = 0;
    };

    template<typename RecordT>
    struct RefIdDataContainer : public RefIdDataContainerBase
    {
        std::vector<std::unique_ptr<Record<RecordT> > > mContainer;

        int getSize() const override;

        const RecordBase& getRecord (int index) const override;

        RecordBase& getRecord (int index) override;

        unsigned int getRecordFlags (int index) const override;

        void appendRecord (const std::string& id, bool base) override;

        void insertRecord (std::unique_ptr<RecordBase> record) override;

        int load (ESM::ESMReader& reader, bool base) override;
        ///< \return index of a loaded record or -1 if no record was loaded

        void erase (int index, int count) override;

        std::string getId (int index) const override;

        void save (int index, ESM::ESMWriter& writer) const override;
    };

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::insertRecord(std::unique_ptr<RecordBase> record)
    {
        assert(record != nullptr);
        // convert base pointer to record type pointer
        std::unique_ptr<Record<RecordT>> typedRecord(&dynamic_cast<Record<RecordT>&>(*record));
        record.release();
        mContainer.push_back(std::move(typedRecord));
    }

    template<typename RecordT>
    int RefIdDataContainer<RecordT>::getSize() const
    {
        return static_cast<int> (mContainer.size());
    }

    template<typename RecordT>
    const RecordBase& RefIdDataContainer<RecordT>::getRecord (int index) const
    {
        return *mContainer.at (index);
    }

    template<typename RecordT>
    RecordBase& RefIdDataContainer<RecordT>::getRecord (int index)
    {
        return *mContainer.at (index);
    }

    template<typename RecordT>
    unsigned int RefIdDataContainer<RecordT>::getRecordFlags (int index) const
    {
        return mContainer.at (index)->get().mRecordFlags;
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::appendRecord (const std::string& id, bool base)
    {
        auto record = std::make_unique<Record<RecordT>>();

        record->mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;

        record->mBase.mId = id;
        record->mModified.mId = id;
        (base ? record->mBase : record->mModified).blank();

        mContainer.push_back (std::move(record));
    }

    template<typename RecordT>
    int RefIdDataContainer<RecordT>::load (ESM::ESMReader& reader, bool base)
    {
        RecordT record;
        bool isDeleted = false;

        record.load(reader, isDeleted);

        int index = 0;
        int numRecords = static_cast<int>(mContainer.size());
        for (; index < numRecords; ++index)
        {
            if (Misc::StringUtils::ciEqual(mContainer[index]->get().mId, record.mId))
            {
                break;
            }
        }

        if (isDeleted)
        {
            if (index == numRecords)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            // Flag the record as Deleted even for a base content file.
            // RefIdData is responsible for its erasure.
            mContainer[index]->mState = RecordBase::State_Deleted;
        }
        else
        {
            if (index == numRecords)
            {
                appendRecord(record.mId, base);
                if (base)
                {
                    mContainer.back()->mBase = record;
                }
                else
                {
                    mContainer.back()->mModified = record;
                }
            }
            else if (!base)
            {
                mContainer[index]->setModified(record);
            }
            else
            {
                // Overwrite
                mContainer[index]->setModified(record);
                mContainer[index]->merge();
            }
        }

        return index;
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::erase (int index, int count)
    {
        if (index<0 || index+count>getSize())
            throw std::runtime_error ("invalid RefIdDataContainer index");

        mContainer.erase (mContainer.begin()+index, mContainer.begin()+index+count);
    }

    template<typename RecordT>
    std::string RefIdDataContainer<RecordT>::getId (int index) const
    {
        return mContainer.at (index)->get().mId;
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::save (int index, ESM::ESMWriter& writer) const
    {
        const Record<RecordT>& record = *mContainer.at(index);

        if (record.isModified() || record.mState == RecordBase::State_Deleted)
        {
            RecordT esmRecord = record.get();
            writer.startRecord(esmRecord.sRecordId, esmRecord.mRecordFlags);
            esmRecord.save(writer, record.mState == RecordBase::State_Deleted);
            writer.endRecord(esmRecord.sRecordId);
        }
    }


    class RefIdData
    {
        public:

            typedef std::pair<int, UniversalId::Type> LocalIndex;

        private:

            RefIdDataContainer<ESM::Activator> mActivators;
            RefIdDataContainer<ESM::Potion> mPotions;
            RefIdDataContainer<ESM::Apparatus> mApparati;
            RefIdDataContainer<ESM::Armor> mArmors;
            RefIdDataContainer<ESM::Book> mBooks;
            RefIdDataContainer<ESM::Clothing> mClothing;
            RefIdDataContainer<ESM::Container> mContainers;
            RefIdDataContainer<ESM::Creature> mCreatures;
            RefIdDataContainer<ESM::Door> mDoors;
            RefIdDataContainer<ESM::Ingredient> mIngredients;
            RefIdDataContainer<ESM::CreatureLevList> mCreatureLevelledLists;
            RefIdDataContainer<ESM::ItemLevList> mItemLevelledLists;
            RefIdDataContainer<ESM::Light> mLights;
            RefIdDataContainer<ESM::Lockpick> mLockpicks;
            RefIdDataContainer<ESM::Miscellaneous> mMiscellaneous;
            RefIdDataContainer<ESM::NPC> mNpcs;
            RefIdDataContainer<ESM::Probe> mProbes;
            RefIdDataContainer<ESM::Repair> mRepairs;
            RefIdDataContainer<ESM::Static> mStatics;
            RefIdDataContainer<ESM::Weapon> mWeapons;

            std::map<std::string, LocalIndex> mIndex;

            std::map<UniversalId::Type, RefIdDataContainerBase *> mRecordContainers;

            void erase (const LocalIndex& index, int count);
            ///< Must not spill over into another type.

            std::string getRecordId(const LocalIndex &index) const;

        public:

            RefIdData();

            LocalIndex globalToLocalIndex (int index) const;

            int localToGlobalIndex (const LocalIndex& index) const;

            LocalIndex searchId(std::string_view id) const;

            void erase (int index, int count);

            void insertRecord (std::unique_ptr<RecordBase> record, CSMWorld::UniversalId::Type type,
                const std::string& id);

            const RecordBase& getRecord (const LocalIndex& index) const;

            RecordBase& getRecord (const LocalIndex& index);

            unsigned int getRecordFlags(const std::string& id) const;

            void appendRecord (UniversalId::Type type, const std::string& id, bool base);

            int getAppendIndex (UniversalId::Type type) const;

            void load (ESM::ESMReader& reader, bool base, UniversalId::Type type);

            int getSize() const;

            std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            void save (int index, ESM::ESMWriter& writer) const;

            //RECORD CONTAINERS ACCESS METHODS
            const RefIdDataContainer<ESM::Book>& getBooks() const;
            const RefIdDataContainer<ESM::Activator>& getActivators() const;
            const RefIdDataContainer<ESM::Potion>& getPotions() const;
            const RefIdDataContainer<ESM::Apparatus>& getApparati() const;
            const RefIdDataContainer<ESM::Armor>& getArmors() const;
            const RefIdDataContainer<ESM::Clothing>& getClothing() const;
            const RefIdDataContainer<ESM::Container>& getContainers() const;
            const RefIdDataContainer<ESM::Creature>& getCreatures() const;
            const RefIdDataContainer<ESM::Door>& getDoors() const;
            const RefIdDataContainer<ESM::Ingredient>& getIngredients() const;
            const RefIdDataContainer<ESM::CreatureLevList>& getCreatureLevelledLists() const;
            const RefIdDataContainer<ESM::ItemLevList>& getItemLevelledList() const;
            const RefIdDataContainer<ESM::Light>& getLights() const;
            const RefIdDataContainer<ESM::Lockpick>& getLocpicks() const;
            const RefIdDataContainer<ESM::Miscellaneous>& getMiscellaneous() const;
            const RefIdDataContainer<ESM::NPC>& getNPCs() const;
            const RefIdDataContainer<ESM::Weapon >& getWeapons() const;
            const RefIdDataContainer<ESM::Probe >& getProbes() const;
            const RefIdDataContainer<ESM::Repair>& getRepairs() const;
            const RefIdDataContainer<ESM::Static>& getStatics() const;

            void copyTo (int index, RefIdData& target) const;
    };
}

#endif
