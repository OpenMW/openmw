#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <components/esm/records.hpp>

#include <components/esm_store/store.hpp>

#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace MWWorld
{
  /// A reference to one object (of any type) in a cell.
  ///
  /// Constructing this with a CellRef instance in the constructor means that
  /// in practice (where D is RefData) the possibly mutable data is copied
  /// across to mData. If later adding data (such as position) to CellRef
  /// this would have to be manually copied across.
  template <typename X, typename D>
  struct LiveCellRef
  {
    LiveCellRef(const ESM::CellRef& cref, const X* b = NULL) : base(b), ref(cref),
                                                          mData(ref) {}


    LiveCellRef(const X* b = NULL) : base(b), mData(ref) {}

    // The object that this instance is based on.
    const X* base;

    /* Information about this instance, such as 3D location and
       rotation and individual type-dependent data.
    */
    ESM::CellRef ref;

    /// runtime-data
    D mData;
  };

  /// A list of cell references
  template <typename X, typename D>
  struct CellRefList
  {
    typedef LiveCellRef<X, D> LiveRef;
    typedef std::list<LiveRef> List;
    List list;

    // Search for the given reference in the given reclist from
    // ESMStore. Insert the reference into the list if a match is
    // found. If not, throw an exception.
    template <typename Y>
    void find(ESM::CellRef &ref, const Y& recList)
    {
      const X* obj = recList.find(ref.refID);
      if(obj == NULL)
        throw std::runtime_error("Error resolving cell reference " + ref.refID);

      list.push_back(LiveRef(ref, obj));
    }

    LiveRef *find (const std::string& name)
    {
        for (typename std::list<LiveRef>::iterator iter (list.begin()); iter!=list.end(); ++iter)
        {
            if (iter->ref.refID==name)
                return &*iter;
        }

        return 0;
    }
  };

  /// A storage struct for one single cell reference.
  template <typename D>
  class CellStore
  {
  public:

    enum State
    {
        State_Unloaded, State_Preloaded, State_Loaded
    };

    CellStore (const ESM::Cell *cell_) : cell (cell_), mState (State_Unloaded)
    {
        mWaterLevel = cell->water;
    }

    const ESM::Cell *cell;
    State mState;
    std::vector<std::string> mIds;

    float mWaterLevel;

    // Lists for each individual object type
    CellRefList<ESM::Activator, D>         activators;
    CellRefList<ESM::Potion, D>            potions;
    CellRefList<ESM::Apparatus, D>         appas;
    CellRefList<ESM::Armor, D>             armors;
    CellRefList<ESM::Book, D>              books;
    CellRefList<ESM::Clothing, D>          clothes;
    CellRefList<ESM::Container, D>         containers;
    CellRefList<ESM::Creature, D>          creatures;
    CellRefList<ESM::Door, D>              doors;
    CellRefList<ESM::Ingredient, D>        ingreds;
    CellRefList<ESM::CreatureLevList, D>   creatureLists;
    CellRefList<ESM::ItemLevList, D>       itemLists;
    CellRefList<ESM::Light, D>        lights;
    CellRefList<ESM::Tool, D>              lockpicks;
    CellRefList<ESM::Miscellaneous, D>              miscItems;
    CellRefList<ESM::NPC, D>               npcs;
    CellRefList<ESM::Probe, D>             probes;
    CellRefList<ESM::Repair, D>            repairs;
    CellRefList<ESM::Static, D>            statics;
    CellRefList<ESM::Weapon, D>            weapons;

    void load (const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        if (mState!=State_Loaded)
        {
            if (mState==State_Preloaded)
                mIds.clear();

            std::cout << "loading cell " << cell->getDescription() << std::endl;

            loadRefs (store, esm);

            mState = State_Loaded;
        }
    }

    void preload (const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        if (mState==State_Unloaded)
        {
            listRefs (store, esm);

            mState = State_Preloaded;
        }
    }

    /// Call functor (ref) for each reference. functor must return a bool. Returning
    /// false will abort the iteration.
    /// \return Iteration completed?
    template<class Functor>
    bool forEach (Functor& functor)
    {
        return
            forEachImp (functor, activators) &&
            forEachImp (functor, potions) &&
            forEachImp (functor, appas) &&
            forEachImp (functor, armors) &&
            forEachImp (functor, books) &&
            forEachImp (functor, clothes) &&
            forEachImp (functor, containers) &&
            forEachImp (functor, creatures) &&
            forEachImp (functor, doors) &&
            forEachImp (functor, ingreds) &&
            forEachImp (functor, creatureLists) &&
            forEachImp (functor, itemLists) &&
            forEachImp (functor, lights) &&
            forEachImp (functor, lockpicks) &&
            forEachImp (functor, miscItems) &&
            forEachImp (functor, npcs) &&
            forEachImp (functor, probes) &&
            forEachImp (functor, repairs) &&
            forEachImp (functor, statics) &&
            forEachImp (functor, weapons);
    }

  private:

    template<class Functor, class List>
    bool forEachImp (Functor& functor, List& list)
    {
        for (typename List::List::iterator iter (list.list.begin()); iter!=list.list.end();
            ++iter)
            if (!functor (iter->ref, iter->mData))
                return false;

        return true;
    }

    /// Run through references and store IDs
    void listRefs(const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
        assert (cell);

        if (cell->context.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Reopen the ESM reader and seek to the right position.
        cell->restore (esm);

        ESM::CellRef ref;

        // Get each reference in turn
        while (cell->getNextRef (esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.refID.begin(), ref.refID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            mIds.push_back (lowerCase);
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void loadRefs(const ESMS::ESMStore &store, ESM::ESMReader &esm)
    {
      assert (cell);

        if (cell->context.filename.empty())
            return; // this is a dynamically generated cell -> skipping.

      // Reopen the ESM reader and seek to the right position.
      cell->restore(esm);

      ESM::CellRef ref;

      // Get each reference in turn
      while(cell->getNextRef(esm, ref))
        {
            std::string lowerCase;

            std::transform (ref.refID.begin(), ref.refID.end(), std::back_inserter (lowerCase),
                (int(*)(int)) std::tolower);

            int rec = store.find(ref.refID);

            ref.refID = lowerCase;

          /* We can optimize this further by storing the pointer to the
             record itself in store.all, so that we don't need to look it
             up again here. However, never optimize. There are infinite
             opportunities to do that later.
           */
          switch(rec)
            {
            case ESM::REC_ACTI: activators.find(ref, store.activators); break;
            case ESM::REC_ALCH: potions.find(ref, store.potions); break;
            case ESM::REC_APPA: appas.find(ref, store.appas); break;
            case ESM::REC_ARMO: armors.find(ref, store.armors); break;
            case ESM::REC_BOOK: books.find(ref, store.books); break;
            case ESM::REC_CLOT: clothes.find(ref, store.clothes); break;
            case ESM::REC_CONT: containers.find(ref, store.containers); break;
            case ESM::REC_CREA: creatures.find(ref, store.creatures); break;
            case ESM::REC_DOOR: doors.find(ref, store.doors); break;
            case ESM::REC_INGR: ingreds.find(ref, store.ingreds); break;
            case ESM::REC_LEVC: creatureLists.find(ref, store.creatureLists); break;
            case ESM::REC_LEVI: itemLists.find(ref, store.itemLists); break;
            case ESM::REC_LIGH: lights.find(ref, store.lights); break;
            case ESM::REC_LOCK: lockpicks.find(ref, store.lockpicks); break;
            case ESM::REC_MISC: miscItems.find(ref, store.miscItems); break;
            case ESM::REC_NPC_: npcs.find(ref, store.npcs); break;
            case ESM::REC_PROB: probes.find(ref, store.probes); break;
            case ESM::REC_REPA: repairs.find(ref, store.repairs); break;
            case ESM::REC_STAT: statics.find(ref, store.statics); break;
            case ESM::REC_WEAP: weapons.find(ref, store.weapons); break;

            case 0: std::cout << "Cell reference " + ref.refID + " not found!\n"; break;
            default:
              std::cout << "WARNING: Ignoring reference '" << ref.refID << "' of unhandled type\n";
            }
        }
    }

  };
}

#endif
