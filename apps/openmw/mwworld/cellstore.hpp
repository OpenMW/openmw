#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <components/esm/records.hpp>

#include <deque>
#include <algorithm>

#include "refdata.hpp"
#include "esmstore.hpp"

namespace MWWorld
{
    class Ptr;
    class ESMStore;

  /// A reference to one object (of any type) in a cell.
  ///
  /// Constructing this with a CellRef instance in the constructor means that
  /// in practice (where D is RefData) the possibly mutable data is copied
  /// across to mData. If later adding data (such as position) to CellRef
  /// this would have to be manually copied across.
  template <typename X>
  struct LiveCellRef
  {
    LiveCellRef(const ESM::CellRef& cref, const X* b = NULL)
      : mBase(b), mRef(cref), mData(mRef)
    {}

    LiveCellRef(const X* b = NULL)
      : mBase(b), mData(mRef)
    {}

    // The object that this instance is based on.
    const X* mBase;

    /* Information about this instance, such as 3D location and
       rotation and individual type-dependent data.
    */
    ESM::CellRef mRef;

    /// runtime-data
    RefData mData;
  };

  /// A list of cell references
  template <typename X>
  struct CellRefList
  {
    typedef LiveCellRef<X> LiveRef;
    typedef std::map<int,LiveRef> List;
    List mList;

    // Search for the given reference in the given reclist from
    // ESMStore. Insert the reference into the list if a match is
    // found. If not, throw an exception.
    /// Searches for reference of appropriate type in given ESMStore.
    /// If reference exists, loads it into container, throws an exception
    /// on miss
    void load(ESM::CellRef &ref, const MWWorld::ESMStore &esmStore)
    {
        // for throwing exception on unhandled record type
        const MWWorld::Store<X> &store = esmStore.get<X>();
        const X *ptr = store.search(ref.mRefID);

        /// \note no longer redundant - changed to Store<X>::search(), don't throw
        ///  an exception on miss, try to continue (that's how MW does it, anyway)
        if (ptr == NULL) {
            std::cout << "Warning: could not resolve cell reference " << ref.mRefID << ", trying to continue anyway" << std::endl;
        } else
          mList[ref.mRefnum] = LiveRef(ref, ptr);
    }

    LiveRef *find (const std::string& name)
    {
        for (typename std::map<int,LiveRef>::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
        {
            if (iter->second.mData.getCount() > 0 && iter->second.mRef.mRefID == name)
                return &iter->second;
        }

        return 0;
    }

    LiveRef &insert(const LiveRef &item) {
        mList[item.mRef.mRefnum] = item;
        return mList[item.mRef.mRefnum];
    }
  };

  /// A list of container references. These references do not track their mRefnumber.
  /// Otherwise, taking 1 of 20 instances of an object would produce multiple objects
  /// with the same reference.
  // TODO: Check how Morrowind does this! Maybe auto-generate references on drop.
  template <typename X>
  struct ContainerRefList
  {
    typedef LiveCellRef<X> LiveRef;
    typedef std::list<LiveRef> List;
    List mList;

    /// Searches for reference of appropriate type in given ESMStore.
    /// If reference exists, loads it into container, throws an exception
    /// on miss
    void load(ESM::CellRef &ref, const MWWorld::ESMStore &esmStore)
    {
        // for throwing exception on unhandled record type
        const MWWorld::Store<X> &store = esmStore.get<X>();
        const X *ptr = store.find(ref.mRefID);

        /// \note redundant because Store<X>::find() throws exception on miss
        if (ptr == NULL) {
            throw std::runtime_error("Error resolving cell reference " + ref.mRefID);
        }
        mList.push_back(LiveRef(ref, ptr));
    }

    LiveRef *find (const std::string& name)
    {
        for (typename std::list<LiveRef>::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
        {
            if (iter->mData.getCount() > 0 && iter->mRef.mRefID == name)
                return &*iter;
        }

        return 0;
    }

    LiveRef &insert(const LiveRef &item) {
        mList.push_back(item);
        return mList.back();
    }
  };

  /// A storage struct for one single cell reference.
  class CellStore
  {
  public:

    enum State
    {
        State_Unloaded, State_Preloaded, State_Loaded
    };

    CellStore (const ESM::Cell *cell_);

    const ESM::Cell *mCell;
    State mState;
    std::vector<std::string> mIds;

    float mWaterLevel;

    // Lists for each individual object type
    CellRefList<ESM::Activator>         mActivators;
    CellRefList<ESM::Potion>            mPotions;
    CellRefList<ESM::Apparatus>         mAppas;
    CellRefList<ESM::Armor>             mArmors;
    CellRefList<ESM::Book>              mBooks;
    CellRefList<ESM::Clothing>          mClothes;
    CellRefList<ESM::Container>         mContainers;
    CellRefList<ESM::Creature>          mCreatures;
    CellRefList<ESM::Door>              mDoors;
    CellRefList<ESM::Ingredient>        mIngreds;
    CellRefList<ESM::CreatureLevList>   mCreatureLists;
    CellRefList<ESM::ItemLevList>       mItemLists;
    CellRefList<ESM::Light>             mLights;
    CellRefList<ESM::Tool>              mLockpicks;
    CellRefList<ESM::Miscellaneous>     mMiscItems;
    CellRefList<ESM::NPC>               mNpcs;
    CellRefList<ESM::Probe>             mProbes;
    CellRefList<ESM::Repair>            mRepairs;
    CellRefList<ESM::Static>            mStatics;
    CellRefList<ESM::Weapon>            mWeapons;

    void load (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

    void preload (const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

    /// Call functor (ref) for each reference. functor must return a bool. Returning
    /// false will abort the iteration.
    /// \return Iteration completed?
    template<class Functor>
    bool forEach (Functor& functor)
    {
        return
            forEachImp (functor, mActivators) &&
            forEachImp (functor, mPotions) &&
            forEachImp (functor, mAppas) &&
            forEachImp (functor, mArmors) &&
            forEachImp (functor, mBooks) &&
            forEachImp (functor, mClothes) &&
            forEachImp (functor, mContainers) &&
            forEachImp (functor, mCreatures) &&
            forEachImp (functor, mDoors) &&
            forEachImp (functor, mIngreds) &&
            forEachImp (functor, mCreatureLists) &&
            forEachImp (functor, mItemLists) &&
            forEachImp (functor, mLights) &&
            forEachImp (functor, mLockpicks) &&
            forEachImp (functor, mMiscItems) &&
            forEachImp (functor, mNpcs) &&
            forEachImp (functor, mProbes) &&
            forEachImp (functor, mRepairs) &&
            forEachImp (functor, mStatics) &&
            forEachImp (functor, mWeapons);
    }

    bool operator==(const CellStore &cell) {
        return  mCell->mName == cell.mCell->mName &&
                mCell->mData.mX == cell.mCell->mData.mX &&
                mCell->mData.mY == cell.mCell->mData.mY;
    }

    bool operator!=(const CellStore &cell) {
        return !(*this == cell);
    }

    bool isExterior() const {
        return mCell->isExterior();
    }

  private:

    template<class Functor, class List>
    bool forEachImp (Functor& functor, List& list)
    {
        for (typename List::List::iterator iter (list.mList.begin()); iter!=list.mList.end();
            ++iter)
            if (!functor (iter->second.mRef, iter->second.mData))
                return false;

        return true;
    }

    /// Run through references and store IDs
    void listRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

    void loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);
  };
}

#endif
