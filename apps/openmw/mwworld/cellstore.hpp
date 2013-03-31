#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <components/esm/records.hpp>

#include <deque>
#include <algorithm>

#include "refdata.hpp"
#include "esmstore.hpp"

struct C;
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
  
  template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum);

  /// A list of cell references
  template <typename X>
  struct CellRefList
  {
    typedef LiveCellRef<X> LiveRef;
    typedef std::list<LiveRef> List;
    List mList;

    // Search for the given reference in the given reclist from
    // ESMStore. Insert the reference into the list if a match is
    // found. If not, throw an exception.
    // Moved to cpp file, as we require a custom compare operator for it,
    // and the build will fail with an ugly three-way cyclic header dependence
    // so we need to pass the instantiation of the method to the lnker, when
    // all methods are known.
    void load(ESM::CellRef &ref, const MWWorld::ESMStore &esmStore);

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
    CellRefList<ESM::Lockpick>          mLockpicks;
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
            if (!functor (iter->mRef, iter->mData))
                return false;

        return true;
    }

    /// Run through references and store IDs
    void listRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);

    void loadRefs(const MWWorld::ESMStore &store, std::vector<ESM::ESMReader> &esm);
  };
}

#endif
