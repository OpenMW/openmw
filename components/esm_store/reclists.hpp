#ifndef _GAME_ESM_RECLISTS_H
#define _GAME_ESM_RECLISTS_H

#include "components/esm/records.hpp"
#include <map>
#include <string>
#include <assert.h>

namespace ESMS
{
  using namespace ESM;

  struct RecList
  {
    virtual void load(ESMReader &esm, const std::string &id) = 0;
    virtual int getSize() = 0;
  };

  typedef std::map<int,RecList*> RecListList;

  template <typename X>
  struct RecListT : RecList
  {
    typedef std::map<std::string,X> MapType;

    MapType list;

    // Load one object of this type
    void load(ESMReader &esm, const std::string &id)
    {
      list[id].load(esm);
    }

    // Find the given object ID, or return NULL if not found.
    const X* find(const std::string &id) const
    {
      if(list.find(id) == list.end())
        return NULL;
      return &list.find(id)->second;
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

    void load(ESMReader &esm, const std::string &id)
    {
      X& ref = list[id];

      ref.id = id;
      ref.load(esm);
    }

    int getSize() { return list.size(); }
  };

  // Cells aren't simply indexed by name. Exterior cells are treated
  // separately.
  struct CellList : RecList
  {
    // Total cell count. Used for statistics.
    int count;
    CellList() : count(0) {}
    int getSize() { return count; }

    // List of interior cells. Indexed by cell name.
    typedef std::map<std::string,Cell*> IntCells;
    IntCells intCells;

    // List of exterior cells. Indexed as extCells[gridX][gridY].
    typedef std::map<int, Cell*> ExtCellsCol;
    typedef std::map<int, ExtCellsCol> ExtCells;
    ExtCells extCells;

    ~CellList()
    {
      for (IntCells::iterator it = intCells.begin(); it!=intCells.end(); ++it)
        delete it->second;

      for (ExtCells::iterator it = extCells.begin(); it!=extCells.end(); ++it)
      {
        ExtCellsCol& col = it->second;
        for (ExtCellsCol::iterator it = col.begin(); it!=col.end(); ++it)
        {
          delete it->second;
        }
      }
    }

    const Cell* findInt(const std::string &id) const
    {
      IntCells::const_iterator it = intCells.find(id);

      if(it == intCells.end())
        return NULL;

      return it->second;
    }

    void load(ESMReader &esm, const std::string &id)
    {
      using namespace std;

      count++;

      // All cells have a name record, even nameless exterior cells.
      Cell *cell = new Cell;
      cell->name = id;

      // The cell itself takes care of all the hairy details
      cell->load(esm);

      if(cell->data.flags & Cell::Interior)
        {
          // Store interior cell by name
          intCells[id] = cell;
        }
      else
        {
          // Store exterior cells by grid position
          extCells[cell->data.gridX][cell->data.gridY] = cell;
        }
    }
  };

  /* We need special lists for:

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
