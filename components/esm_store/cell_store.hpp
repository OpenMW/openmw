#ifndef _GAME_CELL_STORE_H
#define _GAME_CELL_STORE_H

/*
  Cell storage.

  Used to load, look up and store all references in a single cell.

  Depends on esm/loadcell.hpp (loading from ESM) and esm_store.hpp
  (looking up references.) Neither of these modules depend on us.
 */

#include "store.hpp"
#include "components/esm/records.hpp"
#include "components/esm/loadcell.hpp"
#include <libs/mangle/tools/str_exception.hpp>
#include <list>

#include <iostream>
#include "libs/mangle/tools/str_exception.hpp"

namespace ESMS
{
  using namespace ESM;

  /// A reference to one object (of any type) in a cell.
  template <typename X, typename D>
  struct LiveCellRef
  {
    // The object that this instance is based on.
    const X* base;

    /* Information about this instance, such as 3D location and
       rotation and individual type-dependent data.
    */
    CellRef ref;

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
    void find(CellRef &ref, const Y& recList)
    {
      const X* obj = recList.find(ref.refID);
      if(obj == NULL)
        throw str_exception("Error resolving cell reference " + ref.refID);

      LiveRef lr;
      lr.ref = ref;
      lr.base = obj;

      list.push_back(lr);
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
    CellStore() : cell (0) {}

    const ESM::Cell *cell;

    // Lists for each individual object type
    CellRefList<Activator, D>         activators;
    CellRefList<Potion, D>            potions;
    CellRefList<Apparatus, D>         appas;
    CellRefList<Armor, D>             armors;
    CellRefList<Book, D>              books;
    CellRefList<Clothing, D>          clothes;
    CellRefList<Container, D>         containers;
    CellRefList<Creature, D>          creatures;
    CellRefList<Door, D>              doors;
    CellRefList<Ingredient, D>        ingreds;
    CellRefList<CreatureLevList, D>   creatureLists;
    CellRefList<ItemLevList, D>       itemLists;
    CellRefList<ESM::Light, D>        lights;
    CellRefList<Tool, D>              lockpicks;
    CellRefList<Misc, D>              miscItems;
    CellRefList<NPC, D>               npcs;
    CellRefList<Tool, D>              probes;
    CellRefList<Tool, D>              repairs;
    CellRefList<Static, D>            statics;
    CellRefList<Weapon, D>            weapons;

    /** Look up and load an interior cell from the given ESM data
        storage. */
    void loadInt(const std::string &name, const ESMStore &store, ESMReader &esm)
    {
        std::cout << "loading cell '" << name << "'\n";

        cell = store.cells.findInt(name);

        if(cell == NULL)
            throw str_exception("Cell not found - " + name);

        loadRefs(store, esm);
    } 

    /** Ditto for exterior cell. */
    void loadExt(int X, int Y, const ESMStore &store, ESMReader &esm)
    {
    
    }

  private:
    void loadRefs(const ESMStore &store, ESMReader &esm)
    {
      assert (cell);

      // Reopen the ESM reader and seek to the right position.
      cell->restore(esm);

      CellRef ref;

      // Get each reference in turn
      while(cell->getNextRef(esm, ref))
        {
          int rec = store.find(ref.refID);

          /* We can optimize this further by storing the pointer to the
             record itself in store.all, so that we don't need to look it
             up again here. However, never optimize. There are infinite
             opportunities to do that later.
           */
          switch(rec)
            {
            case REC_ACTI: activators.find(ref, store.activators); break;
            case REC_ALCH: potions.find(ref, store.potions); break;
            case REC_APPA: appas.find(ref, store.appas); break;
            case REC_ARMO: armors.find(ref, store.armors); break;
            case REC_BOOK: books.find(ref, store.books); break;
            case REC_CLOT: clothes.find(ref, store.clothes); break;
            case REC_CONT: containers.find(ref, store.containers); break;
            case REC_CREA: creatures.find(ref, store.creatures); break;
            case REC_DOOR: doors.find(ref, store.doors); break;
            case REC_INGR: ingreds.find(ref, store.ingreds); break;
            case REC_LEVC: creatureLists.find(ref, store.creatureLists); break;
            case REC_LEVI: itemLists.find(ref, store.itemLists); break;
            case REC_LIGH: lights.find(ref, store.lights); break;
            case REC_LOCK: lockpicks.find(ref, store.lockpicks); break;
            case REC_MISC: miscItems.find(ref, store.miscItems); break;
            case REC_NPC_: npcs.find(ref, store.npcs); break;
            case REC_PROB: probes.find(ref, store.probes); break;
            case REC_REPA: repairs.find(ref, store.repairs); break;
            case REC_STAT: statics.find(ref, store.statics); break;
            case REC_WEAP: weapons.find(ref, store.weapons); break;

            case 0: std::cout << "Cell reference " + ref.refID + " not found!\n"; break;
            default:
              std::cout << "WARNING: Ignoring reference '" << ref.refID << "' of unhandled type\n";
            }
        }
    }
    
  };
}

#endif
