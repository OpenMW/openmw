#include "cell_store.hpp"
#include <iostream>
#include "mangle/tools/str_exception.h"

using namespace ESMS;
using namespace std;

void CellStore::loadInt(const std::string &name, const ESMStore &store, ESMReader &esm)
{
  cout << "loading cell '" << name << "'\n";

  const Cell *ref = store.cells.findInt(name);

  if(ref == NULL)
    throw str_exception("Cell not found - " + name);

  loadRefs(*ref, store, esm);
}

void CellStore::loadExt(int X, int Y, const ESMStore &store, ESMReader &esm)
{
}

void CellStore::loadRefs(const Cell &cell, const ESMStore &store, ESMReader &esm)
{
  // Reopen the ESM reader and seek to the right position.
  cell.restore(esm);

  CellRef ref;

  // Get each reference in turn
  while(cell.getNextRef(esm, ref))
    {
      cout << "Reference: " << ref.refID;

      // Check each list in turn. Potential for optimization.
      if(
         activators     .find(ref, store.activators) &&
         potions        .find(ref, store.potions) &&
         appas          .find(ref, store.appas) &&
         armors         .find(ref, store.armors) &&
         books          .find(ref, store.books) &&
         clothes        .find(ref, store.clothes) &&
         containers     .find(ref, store.containers) &&
         creatures      .find(ref, store.creatures) &&
         doors          .find(ref, store.doors) &&
         ingreds        .find(ref, store.ingreds) &&
         creatureLists  .find(ref, store.creatureLists) &&
         itemLists      .find(ref, store.itemLists) &&
         lights         .find(ref, store.lights) &&
         lockpicks      .find(ref, store.lockpicks) &&
         miscItems      .find(ref, store.miscItems) &&
         npcs           .find(ref, store.npcs) &&
         probes         .find(ref, store.probes) &&
         repairs        .find(ref, store.repairs) &&
         statics        .find(ref, store.statics) &&
         weapons        .find(ref, store.weapons)

         ) cout << " (NOT FOUND)";

      cout << endl;
    }
}
