#ifndef _GAME_CELL_STORE_H
#define _GAME_CELL_STORE_H

/*
  Cell storage.

  Used to load, look up and store all references in a single cell.

  Depends on esm/loadcell.hpp (loading from ESM) and esm_store.hpp
  (looking up references.) Neither of these modules depend on us.
 */

#include "store.hpp"
#include "esm/records.hpp"
#include <mangle/tools/str_exception.hpp>
#include <list>

namespace ESMS
{
  using namespace ESM;

  /// A reference to one object (of any type) in a cell.
  template <typename X>
  struct LiveCellRef
  {
    // The object that this instance is based on.
    const X* base;

    /* Information about this instance, such as 3D location and
       rotation and individual type-dependent data.
    */
    CellRef ref;

    /* Pointer to any user-defined or engine-specific object. Eg. a
       Sound object for sound sources.
    */
    void *custom;
  };

  /// A list of cell references
  template <typename X>
  struct CellRefList
  {
    typedef LiveCellRef<X> LiveRef;
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
      lr.custom = NULL;

      list.push_back(lr);
    }
  };

  /// A storage struct for one single cell reference.
  struct CellStore
  {
    // Lists for each individual object type
    CellRefList<Activator>         activators;
    CellRefList<Potion>            potions;
    CellRefList<Apparatus>         appas;
    CellRefList<Armor>             armors;
    CellRefList<Book>              books;
    CellRefList<Clothing>          clothes;
    CellRefList<Container>         containers;
    CellRefList<Creature>          creatures;
    CellRefList<Door>              doors;
    CellRefList<Ingredient>        ingreds;
    CellRefList<CreatureLevList>   creatureLists;
    CellRefList<ItemLevList>       itemLists;
    CellRefList<Light>             lights;
    CellRefList<Tool>              lockpicks;
    CellRefList<Misc>              miscItems;
    CellRefList<NPC>               npcs;
    CellRefList<Tool>              probes;
    CellRefList<Tool>              repairs;
    CellRefList<Static>            statics;
    CellRefList<Weapon>            weapons;

    /** Look up and load an interior cell from the given ESM data
        storage. */
    void loadInt(const std::string &name, const ESMStore &data, ESMReader &esm);

    /** Ditto for exterior cell. */
    void loadExt(int X, int Y, const ESMStore &data, ESMReader &esm);

  private:
    void loadRefs(const Cell &cell, const ESMStore &data, ESMReader &esm);
  };
}

#endif
