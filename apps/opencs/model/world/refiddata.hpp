#ifndef CSM_WOLRD_REFIDDATA_H
#define CSM_WOLRD_REFIDDATA_H

#include <vector>
#include <map>

#include <components/esm/loadacti.hpp>
#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadcont.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loaddoor.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadlevlist.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadlock.hpp>
#include <components/esm/loadprob.hpp>
#include <components/esm/loadrepa.hpp>
#include <components/esm/loadstat.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadmisc.hpp>

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

        virtual void appendRecord (const std::string& id) = 0;

        virtual void load (int index,  ESM::ESMReader& reader, bool base) = 0;

        virtual void erase (int index, int count) = 0;

        virtual std::string getId (int index) const = 0;
    };

    template<typename RecordT>
    struct RefIdDataContainer : public RefIdDataContainerBase
    {
        std::vector<Record<RecordT> > mContainer;

        virtual int getSize() const;

        virtual const RecordBase& getRecord (int index) const;

        virtual RecordBase& getRecord (int index);

        virtual void appendRecord (const std::string& id);

        virtual void load (int index,  ESM::ESMReader& reader, bool base);

        virtual void erase (int index, int count);

        virtual std::string getId (int index) const;
    };

    template<typename RecordT>
    int RefIdDataContainer<RecordT>::getSize() const
    {
        return static_cast<int> (mContainer.size());
    }

    template<typename RecordT>
    const RecordBase& RefIdDataContainer<RecordT>::getRecord (int index) const
    {
        return mContainer.at (index);
    }

    template<typename RecordT>
    RecordBase& RefIdDataContainer<RecordT>::getRecord (int index)
    {
        return mContainer.at (index);
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::appendRecord (const std::string& id)
    {
        Record<RecordT> record;
        record.mModified.mId = id;
        record.mModified.blank();
        record.mState = RecordBase::State_ModifiedOnly;

        mContainer.push_back (record);
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::load (int index,  ESM::ESMReader& reader, bool base)
    {
        (base ? mContainer.at (index).mBase : mContainer.at (index).mModified).load (reader);
    }

    template<typename RecordT>
    void RefIdDataContainer<RecordT>::erase (int index, int count)
    {
        if (index<0 || index+count>=getSize())
            throw std::runtime_error ("invalid RefIdDataContainer index");

        mContainer.erase (mContainer.begin()+index, mContainer.begin()+index+count);
    }

    template<typename RecordT>
    std::string RefIdDataContainer<RecordT>::getId (int index) const
    {
        return mContainer.at (index).get().mId;
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

        public:

            RefIdData();

            LocalIndex globalToLocalIndex (int index) const;

            int localToGlobalIndex (const LocalIndex& index) const;

            LocalIndex searchId (const std::string& id) const;

            void erase (int index, int count);

            const RecordBase& getRecord (const LocalIndex& index) const;

            RecordBase& getRecord (const LocalIndex& index);

            void appendRecord (UniversalId::Type type, const std::string& id);

            int getAppendIndex (UniversalId::Type type) const;

            void load (const LocalIndex& index, ESM::ESMReader& reader, bool base);

            int getSize() const;
    };
}

#endif
