#include "cellstore.hpp"

#include <iostream>

#include <components/esm/cellstate.hpp>
#include <components/esm/cellid.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/objectstate.hpp>
#include <components/esm/lightstate.hpp>
#include <components/esm/containerstate.hpp>
#include <components/esm/npcstate.hpp>
#include <components/esm/creaturestate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "ptr.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "containerstore.hpp"

namespace
{
    template<typename T>
    MWWorld::Ptr searchInContainerList (MWWorld::CellRefList<T>& containerList, const std::string& id)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (containerList.mList.begin());
             iter!=containerList.mList.end(); ++iter)
        {
            MWWorld::Ptr container (&*iter, 0);

            MWWorld::Ptr ptr =
                MWWorld::Class::get (container).getContainerStore (container).search (id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return MWWorld::Ptr();
    }

    template<typename RecordType, typename T>
    void writeReferenceCollection (ESM::ESMWriter& writer,
        const MWWorld::CellRefList<T>& collection)
    {
        if (!collection.mList.empty())
        {
            // references
            for (typename MWWorld::CellRefList<T>::List::const_iterator
                iter (collection.mList.begin());
                iter!=collection.mList.end(); ++iter)
            {
                if (iter->mData.getCount()==0 && iter->mRef.mRefNum.mContentFile==-1)
                    continue; // deleted file that did not came from a content file -> ignore

                RecordType state;
                iter->save (state);

                writer.writeHNT ("OBJE", collection.mList.front().mBase->sRecordId);
                state.save (writer);
            }
        }
    }

    template<typename RecordType, typename T>
    void readReferenceCollection (ESM::ESMReader& reader,
        MWWorld::CellRefList<T>& collection, const std::map<int, int>& contentFileMap)
    {
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        RecordType state;
        state.load (reader);

        std::map<int, int>::const_iterator iter =
            contentFileMap.find (state.mRef.mRefNum.mContentFile);

        if (iter==contentFileMap.end())
            return; // content file has been removed -> skip

        state.mRef.mRefNum.mContentFile = iter->second;

        if (!MWWorld::LiveCellRef<T>::checkState (state))
            return; // not valid anymore with current content files -> skip

        const T *record = esmStore.get<T>().search (state.mRef.mRefID);

        if (!record)
            return;

        for (typename MWWorld::CellRefList<T>::List::iterator iter (collection.mList.begin());
            iter!=collection.mList.end(); ++iter)
            if (iter->mRef.mRefNum==state.mRef.mRefNum)
            {
                // overwrite existing reference
                iter->load (state);
                return;
            }

        // new reference
        MWWorld::LiveCellRef<T> ref (record);
        ref.load (state);
        collection.mList.push_back (ref);
    }
}

namespace MWWorld
{

    template <typename X>
    void CellRefList<X>::load(ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore)
    {
        const MWWorld::Store<X> &store = esmStore.get<X>();

        if (const X *ptr = store.search (ref.mRefID))
        {
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref.mRefNum);

            LiveRef liveCellRef (ref, ptr);

            if (deleted)
                liveCellRef.mData.setCount (0);

            if (iter != mList.end())
                *iter = liveCellRef;
            else
                mList.push_back (liveCellRef);
        }
        else
        {
            std::cerr
                << "Error: could not resolve cell reference " << ref.mRefID
                << " (dropping reference)" << std::endl;
        }
    }

    template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum)
    {
        return (ref.mRef.mRefnum == pRefnum);
    }

    CellStore::CellStore (const ESM::Cell *cell)
      : mCell (cell), mState (State_Unloaded)
    {
        mWaterLevel = cell->mWater;
    }

    void CellStore::load (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        if (mState!=State_Loaded)
        {
            if (mState==State_Preloaded)
                mIds.clear();

            std::cout << "loading cell " << mCell->getDescription() << std::endl;

            loadRefs (store, esm);

            mState = State_Loaded;
        }
    }

    void CellStore::preload (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        if (mState==State_Unloaded)
        {
            listRefs (store, esm);

            mState = State_Preloaded;
        }
    }

    void CellStore::listRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (esm[index], i);

            ESM::CellRef ref;

            // Get each reference in turn
            bool deleted = false;
            while (mCell->getNextRef (esm[index], ref, deleted))
            {
                if (deleted)
                    continue;

                mIds.push_back (Misc::StringUtils::lowerCase (ref.mRefID));
            }
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (esm[index], i);

            ESM::CellRef ref;
            ref.mRefNum.mContentFile = -1;

            // Get each reference in turn
            bool deleted = false;
            while(mCell->getNextRef(esm[index], ref, deleted))
            {
                // Don't load reference if it was moved to a different cell.
                std::string lowerCase = Misc::StringUtils::lowerCase(ref.mRefID);
                ESM::MovedCellRefTracker::const_iterator iter =
                    std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefNum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }

                loadRef (ref, deleted, store);
            }
        }

        // Load moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin(); it != mCell->mLeasedRefs.end(); ++it)
        {
            ESM::CellRef &ref = const_cast<ESM::CellRef&>(*it);

            loadRef (ref, false, store);
        }
    }

    Ptr CellStore::searchInContainer (const std::string& id)
    {
        {
            Ptr ptr = searchInContainerList (mContainers, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        {
            Ptr ptr = searchInContainerList (mCreatures, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        {
            Ptr ptr = searchInContainerList (mNpcs, id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return Ptr();
    }

    void CellStore::loadRef (ESM::CellRef& ref, bool deleted, const ESMStore& store)
    {
        Misc::StringUtils::toLower (ref.mRefID);

        switch (store.find (ref.mRefID))
        {
            case ESM::REC_ACTI: mActivators.load(ref, deleted, store); break;
            case ESM::REC_ALCH: mPotions.load(ref, deleted, store); break;
            case ESM::REC_APPA: mAppas.load(ref, deleted, store); break;
            case ESM::REC_ARMO: mArmors.load(ref, deleted, store); break;
            case ESM::REC_BOOK: mBooks.load(ref, deleted, store); break;
            case ESM::REC_CLOT: mClothes.load(ref, deleted, store); break;
            case ESM::REC_CONT: mContainers.load(ref, deleted, store); break;
            case ESM::REC_CREA: mCreatures.load(ref, deleted, store); break;
            case ESM::REC_DOOR: mDoors.load(ref, deleted, store); break;
            case ESM::REC_INGR: mIngreds.load(ref, deleted, store); break;
            case ESM::REC_LEVC: mCreatureLists.load(ref, deleted, store); break;
            case ESM::REC_LEVI: mItemLists.load(ref, deleted, store); break;
            case ESM::REC_LIGH: mLights.load(ref, deleted, store); break;
            case ESM::REC_LOCK: mLockpicks.load(ref, deleted, store); break;
            case ESM::REC_MISC: mMiscItems.load(ref, deleted, store); break;
            case ESM::REC_NPC_: mNpcs.load(ref, deleted, store); break;
            case ESM::REC_PROB: mProbes.load(ref, deleted, store); break;
            case ESM::REC_REPA: mRepairs.load(ref, deleted, store); break;
            case ESM::REC_STAT: mStatics.load(ref, deleted, store); break;
            case ESM::REC_WEAP: mWeapons.load(ref, deleted, store); break;

            case 0: std::cerr << "Cell reference " + ref.mRefID + " not found!\n"; break;

            default:
                std::cerr
                    << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
        }
    }

    void CellStore::loadState (const ESM::CellState& state)
    {
        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            mWaterLevel = state.mWaterLevel;

        mWaterLevel = state.mWaterLevel;
    }

    void CellStore::saveState (ESM::CellState& state) const
    {
        state.mId = mCell->getCellId();

        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            state.mWaterLevel = mWaterLevel;

        state.mWaterLevel = mWaterLevel;
    }

    void CellStore::writeReferences (ESM::ESMWriter& writer) const
    {
        writeReferenceCollection<ESM::ObjectState> (writer, mActivators);
        writeReferenceCollection<ESM::ObjectState> (writer, mPotions);
        writeReferenceCollection<ESM::ObjectState> (writer, mAppas);
        writeReferenceCollection<ESM::ObjectState> (writer, mArmors);
        writeReferenceCollection<ESM::ObjectState> (writer, mBooks);
        writeReferenceCollection<ESM::ObjectState> (writer, mClothes);
        writeReferenceCollection<ESM::ContainerState> (writer, mContainers);
        writeReferenceCollection<ESM::CreatureState> (writer, mCreatures);
        writeReferenceCollection<ESM::ObjectState> (writer, mDoors);
        writeReferenceCollection<ESM::ObjectState> (writer, mIngreds);
        writeReferenceCollection<ESM::ObjectState> (writer, mCreatureLists);
        writeReferenceCollection<ESM::ObjectState> (writer, mItemLists);
        writeReferenceCollection<ESM::LightState> (writer, mLights);
        writeReferenceCollection<ESM::ObjectState> (writer, mLockpicks);
        writeReferenceCollection<ESM::ObjectState> (writer, mMiscItems);
        writeReferenceCollection<ESM::NpcState> (writer, mNpcs);
        writeReferenceCollection<ESM::ObjectState> (writer, mProbes);
        writeReferenceCollection<ESM::ObjectState> (writer, mRepairs);
        writeReferenceCollection<ESM::ObjectState> (writer, mStatics);
        writeReferenceCollection<ESM::ObjectState> (writer, mWeapons);
    }

    void CellStore::readReferences (ESM::ESMReader& reader,
        const std::map<int, int>& contentFileMap)
    {
        while (reader.isNextSub ("OBJE"))
        {
            unsigned int id = 0;
            reader.getHT (id);

            switch (id)
            {
                case ESM::REC_ACTI:

                    readReferenceCollection<ESM::ObjectState> (reader, mActivators, contentFileMap);
                    break;

                case ESM::REC_ALCH:

                    readReferenceCollection<ESM::ObjectState> (reader, mPotions, contentFileMap);
                    break;

                case ESM::REC_APPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mAppas, contentFileMap);
                    break;

                case ESM::REC_ARMO:

                    readReferenceCollection<ESM::ObjectState> (reader, mArmors, contentFileMap);
                    break;

                case ESM::REC_BOOK:

                    readReferenceCollection<ESM::ObjectState> (reader, mBooks, contentFileMap);
                    break;

                case ESM::REC_CLOT:

                    readReferenceCollection<ESM::ObjectState> (reader, mClothes, contentFileMap);
                    break;

                case ESM::REC_CONT:

                    readReferenceCollection<ESM::ContainerState> (reader, mContainers, contentFileMap);
                    break;

                case ESM::REC_CREA:

                    readReferenceCollection<ESM::CreatureState> (reader, mCreatures, contentFileMap);
                    break;

                case ESM::REC_DOOR:

                    readReferenceCollection<ESM::ObjectState> (reader, mDoors, contentFileMap);
                    break;

                case ESM::REC_INGR:

                    readReferenceCollection<ESM::ObjectState> (reader, mIngreds, contentFileMap);
                    break;

                case ESM::REC_LEVC:

                    readReferenceCollection<ESM::ObjectState> (reader, mCreatureLists, contentFileMap);
                    break;

                case ESM::REC_LEVI:

                    readReferenceCollection<ESM::ObjectState> (reader, mItemLists, contentFileMap);
                    break;

                case ESM::REC_LIGH:

                    readReferenceCollection<ESM::LightState> (reader, mLights, contentFileMap);
                    break;

                case ESM::REC_LOCK:

                    readReferenceCollection<ESM::ObjectState> (reader, mLockpicks, contentFileMap);
                    break;

                case ESM::REC_MISC:

                    readReferenceCollection<ESM::ObjectState> (reader, mMiscItems, contentFileMap);
                    break;

                case ESM::REC_NPC_:

                    readReferenceCollection<ESM::NpcState> (reader, mNpcs, contentFileMap);
                    break;

                case ESM::REC_PROB:

                    readReferenceCollection<ESM::ObjectState> (reader, mProbes, contentFileMap);
                    break;

                case ESM::REC_REPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mRepairs, contentFileMap);
                    break;

                case ESM::REC_STAT:

                    readReferenceCollection<ESM::ObjectState> (reader, mStatics, contentFileMap);
                    break;

                case ESM::REC_WEAP:

                    readReferenceCollection<ESM::ObjectState> (reader, mWeapons, contentFileMap);
                    break;

                default:

                    throw std::runtime_error ("unknown type in cell reference section");
            }
        }
    }
}
