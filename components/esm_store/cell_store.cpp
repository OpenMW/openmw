#include "cell_store.hpp"
#include <iostream>
#include "libs/mangle/tools/str_exception.hpp"

using namespace ESMS;
using namespace std;

void CellStore::loadInt(const std::string &name, const ESMStore &store, ESMReader &esm)
{
  cout << "loading cell '" << name << "'\n";

  cell = store.cells.findInt(name);

  if(cell == NULL)
    throw str_exception("Cell not found - " + name);

  loadRefs(store, esm);
}

void CellStore::loadExt(int X, int Y, const ESMStore &store, ESMReader &esm)
{
}

void CellStore::loadRefs(const ESMStore &store, ESMReader &esm)
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

        case 0: cout << "Cell reference " + ref.refID + " not found!\n"; break;
        default:
          assert(0);
        }
    }

  cout << "Statics in cell: " << statics.list.size() << endl;
}
