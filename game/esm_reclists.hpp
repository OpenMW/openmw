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

    void load(ESMReader &esm)
    {
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
