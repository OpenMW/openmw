#include "cellstore.hpp"
#include "magiceffects.hpp"

#include <algorithm>
#include <fstream>

#include <components/debug/debuglog.hpp>

#include <components/esm/format.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/cellstate.hpp>
#include <components/esm3/containerstate.hpp>
#include <components/esm3/creaturelevliststate.hpp>
#include <components/esm3/creaturestate.hpp>
#include <components/esm3/doorstate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/fogstate.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/esm3/npcstate.hpp>
#include <components/esm3/objectstate.hpp>
#include <components/esm3/readerscache.hpp>

#include <components/esm4/loadachr.hpp>
#include <components/esm4/loadacti.hpp>
#include <components/esm4/loadalch.hpp>
#include <components/esm4/loadammo.hpp>
#include <components/esm4/loadarmo.hpp>
#include <components/esm4/loadbook.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/esm4/loadclot.hpp>
#include <components/esm4/loadcont.hpp>
#include <components/esm4/loadcrea.hpp>
#include <components/esm4/loaddoor.hpp>
#include <components/esm4/loadflor.hpp>
#include <components/esm4/loadfurn.hpp>
#include <components/esm4/loadimod.hpp>
#include <components/esm4/loadingr.hpp>
#include <components/esm4/loadligh.hpp>
#include <components/esm4/loadmisc.hpp>
#include <components/esm4/loadmstt.hpp>
#include <components/esm4/loadnpc.hpp>
#include <components/esm4/loadrefr.hpp>
#include <components/esm4/loadscol.hpp>
#include <components/esm4/loadstat.hpp>
#include <components/esm4/loadterm.hpp>
#include <components/esm4/loadtree.hpp>
#include <components/esm4/loadweap.hpp>
#include <components/esm4/readerutils.hpp>

#include <components/files/openfile.hpp>
#include <components/misc/tuplehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/recharge.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "class.hpp"
#include "containerstore.hpp"
#include "esmstore.hpp"
#include "inventorystore.hpp"
#include "ptr.hpp"
#include "worldmodel.hpp"

namespace
{

    template <typename Record>
    struct RecordToState
    {
        using StateType = ESM::ObjectState;
    };

    template <>
    struct RecordToState<ESM::NPC>
    {
        using StateType = ESM::NpcState;
    };
    template <>
    struct RecordToState<ESM::Creature>
    {
        using StateType = ESM::CreatureState;
    };
    template <>
    struct RecordToState<ESM::Door>
    {
        using StateType = ESM::DoorState;
    };
    template <>
    struct RecordToState<ESM::Container>
    {
        using StateType = ESM::ContainerState;
    };
    template <>
    struct RecordToState<ESM::CreatureLevList>
    {
        using StateType = ESM::CreatureLevListState;
    };

    template <typename T>
    MWWorld::Ptr searchInContainerList(MWWorld::CellRefList<T>& containerList, const ESM::RefId& id)
    {
        for (auto iter(containerList.mList.begin()); iter != containerList.mList.end(); ++iter)
        {
            MWWorld::Ptr container(&*iter, nullptr);

            if (container.getRefData().getCustomData() == nullptr)
                continue;

            MWWorld::Ptr ptr = container.getClass().getContainerStore(container).search(id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return MWWorld::Ptr();
    }

    template <typename T>
    MWWorld::Ptr searchViaActorId(MWWorld::CellRefList<T>& actorList, int actorId, MWWorld::CellStore* cell,
        const std::map<MWWorld::LiveCellRefBase*, MWWorld::CellStore*>& toIgnore)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter(actorList.mList.begin());
             iter != actorList.mList.end(); ++iter)
        {
            MWWorld::Ptr actor(&*iter, cell);

            if (toIgnore.find(&*iter) != toIgnore.end())
                continue;

            if (actor.getClass().getCreatureStats(actor).matchesActorId(actorId) && actor.getCellRef().getCount() > 0)
                return actor;
        }

        return MWWorld::Ptr();
    }

    template <typename T>
    void writeReferenceCollection(ESM::ESMWriter& writer, const MWWorld::CellRefList<T>& collection)
    {
        // references
        for (const MWWorld::LiveCellRef<T>& liveCellRef : collection.mList)
        {
            if (ESM::isESM4Rec(T::sRecordId))
            {
                // TODO: Implement loading/saving of REFR4 and ACHR4 with ESM3 reader/writer.
                continue;
            }
            if (!liveCellRef.mData.hasChanged() && !liveCellRef.mRef.hasChanged() && liveCellRef.mRef.hasContentFile())
            {
                // Reference that came from a content file and has not been changed -> ignore
                continue;
            }
            if (liveCellRef.mRef.getCount() == 0 && !liveCellRef.mRef.hasContentFile())
            {
                // Deleted reference that did not come from a content file -> ignore
                continue;
            }
            using StateType = typename RecordToState<T>::StateType;
            StateType state;
            liveCellRef.save(state);

            // recordId currently unused
            writer.writeHNT("OBJE", collection.mList.front().mBase->sRecordId);

            state.save(writer);
        }
    }

    template <class RecordType, class T>
    void fixRestockingImpl(const T* base, RecordType& state)
    {
        // Workaround for old saves not containing negative quantities
        for (const auto& baseItem : base->mInventory.mList)
        {
            if (baseItem.mCount < 0)
            {
                for (auto& item : state.mInventory.mItems)
                {
                    if (item.mRef.mCount > 0 && baseItem.mItem == item.mRef.mRefID)
                        item.mRef.mCount = -item.mRef.mCount;
                }
            }
        }
    }

    template <class RecordType, class T>
    void fixRestocking(const T* base, RecordType& state)
    {
    }

    template <>
    void fixRestocking<>(const ESM::Creature* base, ESM::CreatureState& state)
    {
        fixRestockingImpl(base, state);
    }

    template <>
    void fixRestocking<>(const ESM::NPC* base, ESM::NpcState& state)
    {
        fixRestockingImpl(base, state);
    }

    template <>
    void fixRestocking<>(const ESM::Container* base, ESM::ContainerState& state)
    {
        fixRestockingImpl(base, state);
    }

    template <typename T>
    void readReferenceCollection(ESM::ESMReader& reader, MWWorld::CellRefList<T>& collection, const ESM::CellRef& cref,
        const MWWorld::ESMStore& esmStore, MWWorld::CellStore* cellstore)
    {
        using StateType = typename RecordToState<T>::StateType;
        StateType state;
        state.mRef = cref;
        state.load(reader);

        // If the reference came from a content file, make sure this content file is loaded
        if (!reader.applyContentFileMapping(state.mRef.mRefNum))
            return; // content file has been removed -> skip

        if (!MWWorld::LiveCellRef<T>::checkState(state))
            return; // not valid anymore with current content files -> skip

        const T* record = esmStore.get<T>().search(state.mRef.mRefID);

        if (!record)
            return;

        if (state.mVersion <= ESM::MaxOldRestockingFormatVersion)
            fixRestocking(record, state);
        if (state.mVersion <= ESM::MaxClearModifiersFormatVersion)
        {
            if constexpr (std::is_same_v<T, ESM::Creature>)
                MWWorld::convertMagicEffects(state.mCreatureStats, state.mInventory);
            else if constexpr (std::is_same_v<T, ESM::NPC>)
                MWWorld::convertMagicEffects(state.mCreatureStats, state.mInventory, &state.mNpcStats);
        }
        else if (state.mVersion <= ESM::MaxOldCreatureStatsFormatVersion)
        {
            if constexpr (std::is_same_v<T, ESM::Creature> || std::is_same_v<T, ESM::NPC>)
            {
                MWWorld::convertStats(state.mCreatureStats);
                MWWorld::convertEnchantmentSlots(state.mCreatureStats, state.mInventory);
            }
        }
        else if (state.mVersion <= ESM::MaxActiveSpellSlotIndexFormatVersion)
        {
            if constexpr (std::is_same_v<T, ESM::Creature> || std::is_same_v<T, ESM::NPC>)
                MWWorld::convertEnchantmentSlots(state.mCreatureStats, state.mInventory);
        }

        if (state.mRef.mRefNum.hasContentFile())
        {
            for (typename MWWorld::CellRefList<T>::List::iterator iter(collection.mList.begin());
                 iter != collection.mList.end(); ++iter)
                if (iter->mRef.getRefNum() == state.mRef.mRefNum && iter->mRef.getRefId() == state.mRef.mRefID)
                {
                    // overwrite existing reference
                    float oldscale = iter->mRef.getScale();
                    iter->load(state);
                    const ESM::Position& oldpos = iter->mRef.getPosition();
                    const ESM::Position& newpos = iter->mData.getPosition();
                    const MWWorld::Ptr ptr(&*iter, cellstore);
                    if ((oldscale != iter->mRef.getScale() || oldpos.asVec3() != newpos.asVec3()
                            || oldpos.rot[0] != newpos.rot[0] || oldpos.rot[1] != newpos.rot[1]
                            || oldpos.rot[2] != newpos.rot[2])
                        && !ptr.getClass().isActor())
                        MWBase::Environment::get().getWorld()->moveObject(ptr, newpos.asVec3());
                    if (!iter->mData.isEnabled())
                    {
                        iter->mData.enable();
                        MWBase::Environment::get().getWorld()->disable(ptr);
                    }
                    MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
                    return;
                }

            // Note: we preserve RefNum when picking up or dropping an item. This includes non-carriable lights 'picked
            // up' through Lua. So if this RefNum is not found in this cell in content files, it doesn't mean that the
            // instance is invalid. But non-storable item are always stored in saves together with their original cell.
            // If a non-storable item references a content file, but is not found in this content file,
            // we should drop it. Likewise if this stack is empty.
            if (!MWWorld::ContainerStore::isStorableType<T>() || !state.mRef.mCount)
            {
                if (state.mRef.mCount)
                    Log(Debug::Warning) << "Warning: Dropping reference to " << state.mRef.mRefID
                                        << " (invalid content file link)";
                return;
            }
        }

        // new reference
        MWWorld::LiveCellRef<T> ref(record);
        ref.load(state);
        collection.mList.push_back(std::move(ref));

        MWWorld::LiveCellRefBase* base = &collection.mList.back();
        MWBase::Environment::get().getWorldModel()->registerPtr(MWWorld::Ptr(base, cellstore));
    }

    // this function allows us to link a CellRefList<T> to the associated recNameInt, and apply a function
    template <typename RecordType, typename Callable>
    static void recNameSwitcher(MWWorld::CellRefList<RecordType>& store, ESM::RecNameInts recnNameInt, Callable&& f)
    {
        if (RecordType::sRecordId == recnNameInt)
        {
            f(store);
        }
    }

    // helper function for forEachInternal
    template <class Visitor, class List>
    bool forEachImp(Visitor& visitor, List& list, MWWorld::CellStore* cellStore)
    {
        for (typename List::List::iterator iter(list.mList.begin()); iter != list.mList.end(); ++iter)
        {
            if (!MWWorld::CellStore::isAccessible(iter->mData, iter->mRef))
                continue;
            if (!visitor(MWWorld::Ptr(&*iter, cellStore)))
                return false;
        }
        return true;
    }
}

namespace MWWorld
{
    namespace
    {
        template <class T>
        bool isEnabled(const T& ref, const ESMStore& store)
        {
            if (ref.mEsp.parent.isZeroOrUnset())
                return true;

            // Disable objects that are linked to an initially disabled parent.
            // Actually when we will start working on Oblivion/Skyrim scripting we will need to:
            //  - use the current state of the parent instead of initial state of the parent
            //  - every time when the parent is enabled/disabled we should also enable/disable
            //        all objects that are linked to it.
            // But for now we assume that the parent remains in its initial state.
            if (const ESM4::Reference* parentRef = store.get<ESM4::Reference>().searchStatic(ref.mEsp.parent))
            {
                const bool parentDisabled = parentRef->mFlags & ESM4::Rec_Disabled;
                const bool inversed = ref.mEsp.flags & ESM4::EnableParent::Flag_Inversed;
                if (parentDisabled != inversed)
                    return false;

                return isEnabled(*parentRef, store);
            }

            return true;
        }
    }

    struct CellStoreImp
    {
        CellStoreTuple mRefLists;

        template <typename T>
        static void assignStoreToIndex(CellStore& stores, CellRefList<T>& refList)
        {
            const std::size_t storeIndex = CellStore::getTypeIndex<T>();
            if (stores.mCellRefLists.size() <= storeIndex)
                stores.mCellRefLists.resize(storeIndex + 1);

            assert(&refList == &std::get<CellRefList<T>>(stores.mCellStoreImp->mRefLists));

            stores.mCellRefLists[storeIndex] = &refList;
        }

        // listing only objects owned by this cell. Internal use only, you probably want to use forEach() so that moved
        // objects are accounted for.
        template <class Visitor>
        static bool forEachInternal(Visitor& visitor, MWWorld::CellStore& cellStore)
        {
            bool returnValue = true;

            Misc::tupleForEach(cellStore.mCellStoreImp->mRefLists, [&visitor, &returnValue, &cellStore](auto& store) {
                returnValue = returnValue && forEachImp(visitor, store, &cellStore);
            });

            return returnValue;
        }
    };

    template <typename X>
    void CellRefList<X>::load(ESM::CellRef& ref, bool deleted, const MWWorld::ESMStore& esmStore)
    {
        const MWWorld::Store<X>& store = esmStore.get<X>();

        if (const X* ptr = store.search(ref.mRefID))
        {
            typename std::list<LiveRef>::iterator iter = std::find(mList.begin(), mList.end(), ref.mRefNum);

            LiveRef liveCellRef(ref, ptr);

            if (deleted)
                liveCellRef.mData.setDeletedByContentFile(true);

            if (iter != mList.end())
                *iter = std::move(liveCellRef);
            else
                mList.push_back(std::move(liveCellRef));
        }
        else
        {
            Log(Debug::Warning) << "Warning: could not resolve cell reference " << ref.mRefID
                                << " (dropping reference)";
        }
    }

    static constexpr bool isESM4ActorRec(unsigned int rec)
    {
        return rec == ESM::REC_NPC_4 || rec == ESM::REC_CREA4;
    }

    template <typename X, typename R>
    static void loadImpl(const R& ref, const MWWorld::ESMStore& esmStore, auto& list)
    {
        const MWWorld::Store<X>& store = esmStore.get<X>();
        const X* ptr = store.search(ref.mBaseObj);
        if (!ptr)
        {
            Log(Debug::Warning) << "Warning: could not resolve cell reference " << ref.mId << " (dropping reference)";
            return;
        }
        LiveCellRef<X> liveCellRef(ref, ptr);
        if (!isEnabled(ref, esmStore))
            liveCellRef.mData.disable();
        list.push_back(std::move(liveCellRef));
    }

    template <typename X>
    void CellRefList<X>::load(const ESM4::Reference& ref, const MWWorld::ESMStore& esmStore)
    {
        if constexpr (ESM::isESM4Rec(X::sRecordId) && !isESM4ActorRec(X::sRecordId))
            loadImpl<X>(ref, esmStore, mList);
    }
    template <typename X>
    void CellRefList<X>::load(const ESM4::ActorCharacter& ref, const MWWorld::ESMStore& esmStore)
    {
        if constexpr (isESM4ActorRec(X::sRecordId))
            loadImpl<X>(ref, esmStore, mList);
    }

    template <typename X>
    bool operator==(const LiveCellRef<X>& ref, int pRefnum)
    {
        return (ref.mRef.mRefnum == pRefnum);
    }

    Ptr CellStore::getCurrentPtr(LiveCellRefBase* ref)
    {
        MovedRefTracker::iterator found = mMovedToAnotherCell.find(ref);
        if (found != mMovedToAnotherCell.end())
            return Ptr(ref, found->second);
        return Ptr(ref, this);
    }

    void CellStore::moveFrom(const Ptr& object, CellStore* from)
    {
        if (mState != State_Loaded)
            load();

        mHasState = true;
        MovedRefTracker::iterator found = mMovedToAnotherCell.find(object.getBase());
        if (found != mMovedToAnotherCell.end())
        {
            // A cell we had previously moved an object to is returning it to us.
            assert(found->second == from);
            mMovedToAnotherCell.erase(found);
        }
        else
        {
            mMovedHere.insert(std::make_pair(object.getBase(), from));
        }
        requestMergedRefsUpdate();
    }

    MWWorld::Ptr CellStore::moveTo(const Ptr& object, CellStore* cellToMoveTo)
    {
        if (cellToMoveTo == this)
            throw std::runtime_error("moveTo: object is already in this cell");

        // We assume that *this is in State_Loaded since we could hardly have reference to a live object otherwise.
        if (mState != State_Loaded)
            throw std::runtime_error(
                "moveTo: can't move object from a non-loaded cell (how did you get this object anyway?)");

        MWBase::Environment::get().getWorldModel()->registerPtr(MWWorld::Ptr(object.getBase(), cellToMoveTo));

        MovedRefTracker::iterator found = mMovedHere.find(object.getBase());
        if (found != mMovedHere.end())
        {
            // Special case - object didn't originate in this cell
            // Move it back to its original cell first
            CellStore* originalCell = found->second;
            assert(originalCell != this);
            originalCell->moveFrom(object, this);

            mMovedHere.erase(found);

            // Now that object is back to its rightful owner, we can move it
            if (cellToMoveTo != originalCell)
            {
                originalCell->moveTo(object, cellToMoveTo);
            }

            requestMergedRefsUpdate();
            return MWWorld::Ptr(object.getBase(), cellToMoveTo);
        }

        cellToMoveTo->moveFrom(object, this);
        mMovedToAnotherCell.insert(std::make_pair(object.getBase(), cellToMoveTo));

        requestMergedRefsUpdate();
        return MWWorld::Ptr(object.getBase(), cellToMoveTo);
    }

    struct MergeVisitor
    {
        MergeVisitor(std::vector<LiveCellRefBase*>& mergeTo,
            const std::map<LiveCellRefBase*, MWWorld::CellStore*>& movedHere,
            const std::map<LiveCellRefBase*, MWWorld::CellStore*>& movedToAnotherCell)
            : mMergeTo(mergeTo)
            , mMovedHere(movedHere)
            , mMovedToAnotherCell(movedToAnotherCell)
        {
        }

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if (mMovedToAnotherCell.find(ptr.getBase()) != mMovedToAnotherCell.end())
                return true;
            mMergeTo.push_back(ptr.getBase());
            return true;
        }

        void merge()
        {
            for (const auto& [base, _] : mMovedHere)
                mMergeTo.push_back(base);
        }

    private:
        std::vector<LiveCellRefBase*>& mMergeTo;

        const std::map<LiveCellRefBase*, MWWorld::CellStore*>& mMovedHere;
        const std::map<LiveCellRefBase*, MWWorld::CellStore*>& mMovedToAnotherCell;
    };

    void CellStore::requestMergedRefsUpdate()
    {
        mRechargingItemsUpToDate = false;
        mMergedRefsNeedsUpdate = true;
    }

    void CellStore::updateMergedRefs() const
    {
        mMergedRefs.clear();
        MergeVisitor visitor(mMergedRefs, mMovedHere, mMovedToAnotherCell);
        CellStoreImp::forEachInternal(visitor, const_cast<CellStore&>(*this));
        visitor.merge();
        mMergedRefsNeedsUpdate = false;
    }

    bool CellStore::movedHere(const MWWorld::Ptr& ptr) const
    {
        if (ptr.isEmpty())
            return false;

        if (mMovedHere.find(ptr.getBase()) != mMovedHere.end())
            return true;

        return false;
    }

    CellStore::CellStore(MWWorld::Cell&& cell, const MWWorld::ESMStore& esmStore, ESM::ReadersCache& readers)
        : mStore(esmStore)
        , mReaders(readers)
        , mCellVariant(std::move(cell))
        , mState(State_Unloaded)
        , mHasState(false)
        , mLastRespawn(0, 0)
        , mCellStoreImp(std::make_unique<CellStoreImp>())
        , mRechargingItemsUpToDate(false)
    {

        std::apply([this](auto&... x) { (CellStoreImp::assignStoreToIndex(*this, x), ...); }, mCellStoreImp->mRefLists);
        mWaterLevel = mCellVariant.getWaterHeight();
    }

    CellStore::~CellStore() = default;

    const MWWorld::Cell* CellStore::getCell() const
    {
        return &mCellVariant;
    }

    CellStore::State CellStore::getState() const
    {
        return mState;
    }

    const std::vector<ESM::RefId>& CellStore::getPreloadedIds() const
    {
        return mIds;
    }

    bool CellStore::hasState() const
    {
        return mHasState;
    }

    bool CellStore::hasId(const ESM::RefId& id) const
    {
        if (mState == State_Unloaded)
            return false;

        if (mState == State_Preloaded)
            return std::binary_search(mIds.begin(), mIds.end(), id);

        return searchConst(id).isEmpty();
    }

    template <typename PtrType>
    struct SearchVisitor
    {
        PtrType mFound;
        const ESM::RefId& mIdToFind;
        SearchVisitor(const ESM::RefId& id)
            : mIdToFind(id)
        {
        }
        bool operator()(const PtrType& ptr)
        {
            if (ptr.getCellRef().getRefId() == mIdToFind)
            {
                mFound = ptr;
                return false;
            }
            return true;
        }
    };

    Ptr CellStore::search(const ESM::RefId& id)
    {
        SearchVisitor<MWWorld::Ptr> searchVisitor(id);
        forEach(searchVisitor);
        return searchVisitor.mFound;
    }

    ConstPtr CellStore::searchConst(const ESM::RefId& id) const
    {
        SearchVisitor<MWWorld::ConstPtr> searchVisitor(id);
        forEachConst(searchVisitor);
        return searchVisitor.mFound;
    }

    Ptr CellStore::searchViaActorId(int id)
    {
        if (Ptr ptr = ::searchViaActorId(get<ESM::NPC>(), id, this, mMovedToAnotherCell); !ptr.isEmpty())
            return ptr;

        if (Ptr ptr = ::searchViaActorId(get<ESM::Creature>(), id, this, mMovedToAnotherCell); !ptr.isEmpty())
            return ptr;

        for (const auto& [base, _] : mMovedHere)
        {
            MWWorld::Ptr actor(base, this);
            if (!actor.getClass().isActor())
                continue;
            if (actor.getClass().getCreatureStats(actor).matchesActorId(id) && actor.getCellRef().getCount() > 0)
                return actor;
        }

        return Ptr();
    }

    class RefNumSearchVisitor
    {
        ESM::RefNum mRefNum;

    public:
        RefNumSearchVisitor(ESM::RefNum refNum)
            : mRefNum(refNum)
        {
        }

        Ptr mFound;

        bool operator()(const Ptr& ptr)
        {
            if (ptr.getCellRef().getRefNum() == mRefNum)
            {
                mFound = ptr;
                return false;
            }
            return true;
        }
    };

    float CellStore::getWaterLevel() const
    {
        if (isExterior())
            return getCell()->getWaterHeight();
        return mWaterLevel;
    }

    void CellStore::setWaterLevel(float level)
    {
        mWaterLevel = level;
        mHasState = true;
    }

    std::size_t CellStore::count() const
    {
        if (mMergedRefsNeedsUpdate)
            updateMergedRefs();
        return mMergedRefs.size();
    }

    void CellStore::load()
    {
        if (mState != State_Loaded)
        {
            if (mState == State_Preloaded)
                mIds.clear();

            loadRefs();

            mState = State_Loaded;
        }
    }

    void CellStore::preload()
    {
        if (mState == State_Unloaded)
        {
            listRefs();

            mState = State_Preloaded;
        }
    }

    void CellStore::listRefs(const ESM::Cell& cell)
    {
        if (cell.mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < cell.mContextList.size(); i++)
        {
            try
            {
                // Reopen the ESM reader and seek to the right position.
                const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
                const ESM::ReadersCache::BusyItem reader = mReaders.get(index);
                cell.restore(*reader, i);

                ESM::CellRef ref;

                // Get each reference in turn
                ESM::MovedCellRef cMRef;
                bool deleted = false;
                bool moved = false;
                while (ESM::Cell::getNextRef(
                    *reader, ref, deleted, cMRef, moved, ESM::Cell::GetNextRefMode::LoadOnlyNotMoved))
                {
                    if (deleted || moved)
                        continue;

                    // Don't list reference if it was moved to a different cell.
                    ESM::MovedCellRefTracker::const_iterator iter
                        = std::find(cell.mMovedRefs.begin(), cell.mMovedRefs.end(), ref.mRefNum);
                    if (iter != cell.mMovedRefs.end())
                    {
                        continue;
                    }

                    mIds.push_back(std::move(ref.mRefID));
                }
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "An error occurred listing references for cell " << getCell()->getDescription()
                                  << ": " << e.what();
            }
        }

        // List moved references, from separately tracked list.
        for (const auto& [ref, deleted] : cell.mLeasedRefs)
        {
            if (!deleted)
                mIds.push_back(ref.mRefID);
        }
    }

    template <typename ReferenceInvocable>
    static void visitCell4References(
        const ESM4::Cell& cell, const ESMStore& esmStore, ESM::ReadersCache& readers, ReferenceInvocable&& invocable)
    {
        for (const ESM4::Reference* ref : esmStore.get<ESM4::Reference>().getByCell(cell.mId))
            invocable(*ref);
    }

    template <typename ReferenceInvocable>
    static void visitCell4ActorReferences(
        const ESM4::Cell& cell, const ESMStore& esmStore, ESM::ReadersCache& readers, ReferenceInvocable&& invocable)
    {
        for (const ESM4::ActorCharacter* ref : esmStore.get<ESM4::ActorCharacter>().getByCell(cell.mId))
            invocable(*ref);
        for (const ESM4::ActorCharacter* ref : esmStore.get<ESM4::ActorCreature>().getByCell(cell.mId))
            invocable(*ref);
    }

    void CellStore::listRefs(const ESM4::Cell& cell)
    {
        visitCell4References(cell, mStore, mReaders, [&](const ESM4::Reference& ref) { mIds.push_back(ref.mBaseObj); });
        visitCell4ActorReferences(
            cell, mStore, mReaders, [&](const ESM4::ActorCharacter& ref) { mIds.push_back(ref.mBaseObj); });
    }

    void CellStore::listRefs()
    {
        ESM::visit([&](auto&& cell) { listRefs(cell); }, mCellVariant);
        std::sort(mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const ESM::Cell& cell, std::map<ESM::RefNum, ESM::RefId>& refNumToID)
    {
        if (cell.mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < cell.mContextList.size(); i++)
        {
            try
            {
                // Reopen the ESM reader and seek to the right position.
                const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
                const ESM::ReadersCache::BusyItem reader = mReaders.get(index);
                cell.restore(*reader, i);

                ESM::CellRef ref;
                // Get each reference in turn
                ESM::MovedCellRef cMRef;
                bool deleted = false;
                bool moved = false;
                while (ESM::Cell::getNextRef(
                    *reader, ref, deleted, cMRef, moved, ESM::Cell::GetNextRefMode::LoadOnlyNotMoved))
                {
                    if (moved)
                        continue;

                    // Don't load reference if it was moved to a different cell.
                    ESM::MovedCellRefTracker::const_iterator iter
                        = std::find(cell.mMovedRefs.begin(), cell.mMovedRefs.end(), ref.mRefNum);
                    if (iter != cell.mMovedRefs.end())
                    {
                        continue;
                    }

                    loadRef(ref, deleted, refNumToID);
                }
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "An error occurred loading references for cell " << getCell()->getDescription()
                                  << ": " << e.what();
            }
        }
        // Load moved references, from separately tracked list.
        for (const auto& leasedRef : cell.mLeasedRefs)
        {
            ESM::CellRef& ref = const_cast<ESM::CellRef&>(leasedRef.first);
            bool deleted = leasedRef.second;

            loadRef(ref, deleted, refNumToID);
        }
    }

    void CellStore::loadRefs(const ESM4::Cell& cell, std::map<ESM::RefNum, ESM::RefId>& refNumToID)
    {
        visitCell4References(cell, mStore, mReaders, [&](const ESM4::Reference& ref) { loadRef(ref); });
        visitCell4ActorReferences(cell, mStore, mReaders, [&](const ESM4::ActorCharacter& ref) { loadRef(ref); });
    }

    void CellStore::loadRefs()
    {
        std::map<ESM::RefNum, ESM::RefId> refNumToID; // used to detect refID modifications

        ESM::visit([&](auto&& cell) { loadRefs(cell, refNumToID); }, mCellVariant);

        requestMergedRefsUpdate();
    }

    bool CellStore::isExterior() const
    {
        return mCellVariant.isExterior();
    }

    bool CellStore::isQuasiExterior() const
    {
        return mCellVariant.isQuasiExterior();
    }

    Ptr CellStore::searchInContainer(const ESM::RefId& id)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (Ptr ptr = searchInContainerList(get<ESM::Container>(), id); !ptr.isEmpty())
            return ptr;

        if (Ptr ptr = searchInContainerList(get<ESM::Creature>(), id); !ptr.isEmpty())
            return ptr;

        if (Ptr ptr = searchInContainerList(get<ESM::NPC>(), id); !ptr.isEmpty())
            return ptr;

        mHasState = oldState;

        return Ptr();
    }

    void CellStore::loadRef(const ESM4::Reference& ref)
    {
        const MWWorld::ESMStore& store = mStore;

        ESM::RecNameInts foundType = static_cast<ESM::RecNameInts>(store.find(ref.mBaseObj));

        Misc::tupleForEach(this->mCellStoreImp->mRefLists, [&ref, &store, foundType](auto& x) {
            recNameSwitcher(x, foundType, [&ref, &store](auto& storeIn) { storeIn.load(ref, store); });
        });
    }

    void CellStore::loadRef(const ESM4::ActorCharacter& ref)
    {
        const MWWorld::ESMStore& store = mStore;
        ESM::RecNameInts foundType = static_cast<ESM::RecNameInts>(store.find(ref.mBaseObj));

        Misc::tupleForEach(this->mCellStoreImp->mRefLists, [&ref, &store, foundType](auto& x) {
            recNameSwitcher(x, foundType, [&ref, &store](auto& storeIn) { storeIn.load(ref, store); });
        });
    }

    void CellStore::loadRef(ESM::CellRef& ref, bool deleted, std::map<ESM::RefNum, ESM::RefId>& refNumToID)
    {
        const MWWorld::ESMStore& store = mStore;

        auto it = refNumToID.find(ref.mRefNum);
        if (it != refNumToID.end())
        {
            if (it->second != ref.mRefID)
            {
                // refID was modified, make sure we don't end up with duplicated refs
                ESM::RecNameInts foundType = static_cast<ESM::RecNameInts>(store.find(it->second));
                if (foundType != 0)
                {
                    Misc::tupleForEach(this->mCellStoreImp->mRefLists, [&ref, foundType](auto& x) {
                        recNameSwitcher(x, foundType, [&ref](auto& storeIn) { storeIn.remove(ref.mRefNum); });
                    });
                }
            }
        }

        ESM::RecNameInts foundType = static_cast<ESM::RecNameInts>(store.find(ref.mRefID));
        bool handledType = false;
        if (foundType != 0)
        {
            Misc::tupleForEach(
                this->mCellStoreImp->mRefLists, [&ref, &deleted, &store, foundType, &handledType](auto& x) {
                    recNameSwitcher(x, foundType, [&ref, &deleted, &store, &handledType](auto& storeIn) {
                        handledType = true;
                        storeIn.load(ref, deleted, store);
                    });
                });
        }
        else
        {
            Log(Debug::Error) << "Cell reference " << ref.mRefID << " is not found!";
            return;
        }

        if (!handledType)
        {
            Log(Debug::Error) << "Error: Ignoring reference " << ref.mRefID << " of unhandled type";
            return;
        }

        refNumToID[ref.mRefNum] = ref.mRefID;
    }

    void CellStore::loadState(const ESM::CellState& state)
    {
        mHasState = true;

        if (!mCellVariant.isExterior() && mCellVariant.hasWater())
            mWaterLevel = state.mWaterLevel;

        mLastRespawn = MWWorld::TimeStamp(state.mLastRespawn);
    }

    void CellStore::saveState(ESM::CellState& state) const
    {
        state.mId = mCellVariant.getId();

        if (!mCellVariant.isExterior() && mCellVariant.hasWater())
            state.mWaterLevel = mWaterLevel;
        state.mIsInterior = !mCellVariant.isExterior();
        state.mHasFogOfWar = (mFogState.get() ? 1 : 0);
        state.mLastRespawn = mLastRespawn.toEsm();
    }

    void CellStore::writeFog(ESM::ESMWriter& writer) const
    {
        if (mFogState.get())
        {
            mFogState->save(writer, !mCellVariant.isExterior());
        }
    }

    void CellStore::readFog(ESM::ESMReader& reader)
    {
        mFogState = std::make_unique<ESM::FogState>();
        mFogState->load(reader);
    }

    void CellStore::writeReferences(ESM::ESMWriter& writer) const
    {
        Misc::tupleForEach(this->mCellStoreImp->mRefLists,
            [&writer](auto& cellRefList) { writeReferenceCollection(writer, cellRefList); });

        for (const auto& [base, store] : mMovedToAnotherCell)
        {
            ESM::RefNum refNum = base->mRef.getRefNum();
            if (base->isDeleted() && !refNum.hasContentFile())
                continue; // filtered out in writeReferenceCollection
            ESM::RefId movedTo = store->getCell()->getId();

            writer.writeFormId(refNum, true, "MVRF");
            writer.writeCellId(movedTo);
        }
    }

    void CellStore::readReferences(ESM::ESMReader& reader, GetCellStoreCallback* callback)
    {
        mHasState = true;

        while (reader.isNextSub("OBJE"))
        {
            unsigned int unused;
            reader.getHT(unused);

            // load the RefID first so we know what type of object it is
            ESM::CellRef cref;
            cref.loadId(reader, true);

            int type = mStore.find(cref.mRefID);
            if (type == 0)
            {
                Log(Debug::Warning) << "Dropping reference to " << cref.mRefID << " (object no longer exists)";
                // Skip until the next OBJE or MVRF
                while (reader.hasMoreSubs() && !reader.peekNextSub("OBJE") && !reader.peekNextSub("MVRF"))
                {
                    reader.getSubName();
                    reader.skipHSub();
                }
                continue;
            }

            if (type != 0)
            {
                bool foundCorrespondingStore = false;
                Misc::tupleForEach(
                    this->mCellStoreImp->mRefLists, [&reader, this, &cref, &foundCorrespondingStore, type](auto&& x) {
                        recNameSwitcher(x, static_cast<ESM::RecNameInts>(type),
                            [&reader, this, &cref, &foundCorrespondingStore](auto& store) {
                                foundCorrespondingStore = true;
                                readReferenceCollection(reader, store, cref, mStore, this);
                            });
                    });

                if (!foundCorrespondingStore)
                    throw std::runtime_error("unknown type in cell reference section");
            }
        }

        while (reader.isNextSub("MVRF"))
        {
            reader.cacheSubName();
            ESM::RefNum refnum = reader.getFormId(true, "MVRF");
            ESM::RefId movedToId = reader.getCellId();
            if (!reader.applyContentFileMapping(refnum))
            {
                Log(Debug::Warning) << "Warning: Dropping moved ref tag for " << refnum.mIndex
                                    << " (content file no longer exists)";
                continue;
            }

            // Search for the reference. It might no longer exist if its content file was changed.
            Ptr movedRef = MWBase::Environment::get().getWorldModel()->getPtr(refnum);
            if (movedRef.isEmpty())
            {
                Log(Debug::Warning) << "Warning: Dropping moved ref tag for " << refnum.mIndex
                                    << " (moved object no longer exists)";
                continue;
            }

            CellStore* otherCell = callback->getCellStore(movedToId);

            if (otherCell == nullptr)
            {
                Log(Debug::Warning) << "Warning: Dropping moved ref tag for " << movedRef.getCellRef().getRefId()
                                    << " (target cell " << movedToId
                                    << " no longer exists). Reference moved back to its original location.";
                // Note by dropping tag the object will automatically re-appear in its original cell, though
                // potentially at inapproriate coordinates. Restore original coordinates:
                movedRef.getRefData().setPosition(movedRef.getCellRef().getPosition());
                continue;
            }

            if (otherCell == this)
            {
                // Should never happen unless someone's tampering with files.
                Log(Debug::Warning) << "Found invalid moved ref, ignoring";
                continue;
            }

            moveTo(movedRef, otherCell);
        }

        requestMergedRefsUpdate();
    }

    void CellStore::setFog(std::unique_ptr<ESM::FogState>&& fog)
    {
        mFogState = std::move(fog);
    }

    ESM::FogState* CellStore::getFog() const
    {
        return mFogState.get();
    }

    static void clearCorpse(const MWWorld::Ptr& ptr, const ESMStore& store)
    {
        const MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
        static const float fCorpseClearDelay
            = store.get<ESM::GameSetting>().find("fCorpseClearDelay")->mValue.getFloat();
        if (creatureStats.isDead() && creatureStats.isDeathAnimationFinished() && !ptr.getClass().isPersistent(ptr)
            && creatureStats.getTimeOfDeath() + fCorpseClearDelay
                <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            MWBase::Environment::get().getWorld()->deleteObject(ptr);
        }
    }

    void CellStore::rest(double hours)
    {
        if (mState == State_Loaded)
        {
            for (MWWorld::LiveCellRef<ESM::Creature>& creature : get<ESM::Creature>().mList)
            {
                Ptr ptr = getCurrentPtr(&creature);
                if (!ptr.isEmpty() && ptr.getCellRef().getCount() > 0)
                {
                    MWBase::Environment::get().getMechanicsManager()->restoreDynamicStats(ptr, hours, true);
                }
            }
            for (MWWorld::LiveCellRef<ESM::NPC>& npc : get<ESM::NPC>().mList)
            {
                Ptr ptr = getCurrentPtr(&npc);
                if (!ptr.isEmpty() && ptr.getCellRef().getCount() > 0)
                {
                    MWBase::Environment::get().getMechanicsManager()->restoreDynamicStats(ptr, hours, true);
                }
            }
        }
    }

    void CellStore::recharge(float duration)
    {
        if (duration <= 0)
            return;

        if (mState == State_Loaded)
        {
            for (MWWorld::LiveCellRef<ESM::Creature>& creature : get<ESM::Creature>().mList)
            {
                Ptr ptr = getCurrentPtr(&creature);
                if (!ptr.isEmpty() && ptr.getCellRef().getCount() > 0)
                {
                    ptr.getClass().getContainerStore(ptr).rechargeItems(duration);
                }
            }
            for (MWWorld::LiveCellRef<ESM::NPC>& npc : get<ESM::NPC>().mList)
            {
                Ptr ptr = getCurrentPtr(&npc);
                if (!ptr.isEmpty() && ptr.getCellRef().getCount() > 0)
                {
                    ptr.getClass().getContainerStore(ptr).rechargeItems(duration);
                }
            }
            for (MWWorld::LiveCellRef<ESM::Container>& container : get<ESM::Container>().mList)
            {
                Ptr ptr = getCurrentPtr(&container);
                if (!ptr.isEmpty() && ptr.getRefData().getCustomData() != nullptr && ptr.getCellRef().getCount() > 0
                    && ptr.getClass().getContainerStore(ptr).isResolved())
                {
                    ptr.getClass().getContainerStore(ptr).rechargeItems(duration);
                }
            }

            rechargeItems(duration);
        }
    }

    void CellStore::respawn()
    {
        if (mState == State_Loaded)
        {
            static const int iMonthsToRespawn
                = mStore.get<ESM::GameSetting>().find("iMonthsToRespawn")->mValue.getInteger();
            if (MWBase::Environment::get().getWorld()->getTimeStamp() - mLastRespawn > 24 * 30 * iMonthsToRespawn)
            {
                mLastRespawn = MWBase::Environment::get().getWorld()->getTimeStamp();
                for (CellRefList<ESM::Container>::List::iterator it(get<ESM::Container>().mList.begin());
                     it != get<ESM::Container>().mList.end(); ++it)
                {
                    Ptr ptr = getCurrentPtr(&*it);
                    ptr.getClass().respawn(ptr);
                }
            }

            for (CellRefList<ESM::Creature>::List::iterator it(get<ESM::Creature>().mList.begin());
                 it != get<ESM::Creature>().mList.end(); ++it)
            {
                Ptr ptr = getCurrentPtr(&*it);
                clearCorpse(ptr, mStore);
                ptr.getClass().respawn(ptr);
            }
            for (CellRefList<ESM::NPC>::List::iterator it(get<ESM::NPC>().mList.begin());
                 it != get<ESM::NPC>().mList.end(); ++it)
            {
                Ptr ptr = getCurrentPtr(&*it);
                clearCorpse(ptr, mStore);
                ptr.getClass().respawn(ptr);
            }
            forEachType<ESM::CreatureLevList>([](Ptr ptr) {
                // no need to clearCorpse, handled as part of get<ESM::Creature>()
                if (!ptr.mRef->isDeleted())
                    ptr.getClass().respawn(ptr);
                return true;
            });
        }
    }

    void MWWorld::CellStore::rechargeItems(float duration)
    {
        if (!mRechargingItemsUpToDate)
        {
            updateRechargingItems();
            mRechargingItemsUpToDate = true;
        }
        for (const auto& [item, charge] : mRechargingItems)
        {
            MWMechanics::rechargeItem(item, charge, duration);
        }
    }

    void MWWorld::CellStore::updateRechargingItems()
    {
        mRechargingItems.clear();

        const auto update = [this](auto& list) {
            for (auto& item : list)
            {
                Ptr ptr = getCurrentPtr(&item);
                if (!ptr.isEmpty() && ptr.getCellRef().getCount() > 0)
                {
                    checkItem(ptr);
                }
            }
        };

        update(get<ESM::Weapon>().mList);
        update(get<ESM::Armor>().mList);
        update(get<ESM::Clothing>().mList);
        update(get<ESM::Book>().mList);
    }

    void MWWorld::CellStore::checkItem(const Ptr& ptr)
    {
        const ESM::RefId& enchantmentId = ptr.getClass().getEnchantment(ptr);
        if (enchantmentId.empty())
            return;

        const ESM::Enchantment* enchantment = mStore.get<ESM::Enchantment>().search(enchantmentId);
        if (!enchantment)
        {
            Log(Debug::Warning) << "Warning: Can't find enchantment " << enchantmentId << " on item "
                                << ptr.getCellRef().getRefId();
            return;
        }

        if (enchantment->mData.mType == ESM::Enchantment::WhenUsed
            || enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
            mRechargingItems.emplace_back(
                ptr.getBase(), static_cast<float>(MWMechanics::getEnchantmentCharge(*enchantment)));
    }

    Ptr MWWorld::CellStore::getMovedActor(int actorId) const
    {
        for (const auto& [cellRef, cell] : mMovedToAnotherCell)
        {
            if (cellRef->mClass->isActor() && cellRef->mData.getCustomData())
            {
                Ptr actor(cellRef, cell);
                if (actor.getClass().getCreatureStats(actor).getActorId() == actorId)
                    return actor;
            }
        }
        return {};
    }

    Ptr CellStore::getPtr(ESM::RefId id)
    {
        if (mState == CellStore::State_Unloaded)
            preload();

        if (mState == CellStore::State_Preloaded)
        {
            if (!std::binary_search(mIds.begin(), mIds.end(), id))
                return Ptr();
            load();
        }

        Ptr ptr = search(id);

        if (!ptr.isEmpty() && isAccessible(ptr.getRefData(), ptr.getCellRef()))
            return ptr;

        return Ptr();
    }
}
