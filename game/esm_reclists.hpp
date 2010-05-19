#ifndef _GAME_ESM_RECLISTS_H
#define _GAME_ESM_RECLISTS_H

#include "esm/records.hpp"
#include <map>
#include <string>

namespace ESMS
{
  using namespace ESM;

  struct RecList
  {
    virtual void load(ESMReader &esm) = 0;
    virtual int getSize() = 0;
  };

  typedef std::map<int,RecList*> RecListList;

  template <typename X>
  struct RecListT : RecList
  {
    typedef std::map<std::string,X> MapType;

    MapType list;

    void load(ESMReader &esm)
    {
      std::string id = esm.getHNString("NAME");

      X &ref = list[id];
      ref.load(esm);
    }

    int getSize() { return list.size(); }
  };

  // The only difference to the above is a slight change to the load()
  // function. We might merge these together later, and store the id
  // in all the structs.
  template <typename X>
  struct RecIDListT : RecList
  {
    typedef std::map<std::string,X> MapType;

    MapType list;

    void load(ESMReader &esm)
    {
      std::string id = esm.getHNString("NAME");

      X &ref = list[id];
      ref.id = id;
      ref.load(esm);
    }

    int getSize() { return list.size(); }
  };

  // Cells aren't simply indexed by name. Exterior cells are treated
  // separately.
  struct CellList : RecList
  {
    // Just count them for now
    int count;

    CellList() : count(0) {}

    /*
      What to do here:

      load() reads the appropriate records to determine if this is an
      interior or exterior cell. The old D code should be straight
      forward to port here. Unlike the lists above, this struct
      contains two lists, one for each cell type. We will have to hack
      around again to get good indexing of exterior cells, but I think
      a hash thingie like we did in D will work. An alternative is
      just a map<map<>>, so we can do ext_cells[X][Y].whatever. Hmm, I
      think I like that better actually.
     */

    void load(ESMReader &esm)
    {
      // All cells have a name record, even nameless exterior cells.
      std::string id = esm.getHNString("NAME");

      using namespace std;
      cout << id << endl;

      count++;
      esm.skipRecord();
    }

    int getSize() { return count; }
  };

  /* We need special lists for:

     Cells (in progress)
     Magic effects
     Skills
     Dialog / Info combo
     Scripts
     Land
     Path grids
     Land textures
  */
}
#endif
