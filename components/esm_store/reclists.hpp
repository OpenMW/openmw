#ifndef _GAME_ESM_RECLISTS_H
#define _GAME_ESM_RECLISTS_H

#include "components/esm/records.hpp"
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <stdexcept>
#include <iterator>
#include    <boost/algorithm/string.hpp>



using namespace boost::algorithm;

namespace ESMS
{
  using namespace ESM;

  struct RecList
  {
    virtual ~RecList() {}

    virtual void load(ESMReader &esm, const std::string &id) = 0;
    virtual int getSize() = 0;
    virtual void listIdentifier (std::vector<std::string>& identifier) const = 0;

    static std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }
  };

  typedef std::map<int,RecList*> RecListList;

  template <typename X>
  struct RecListT : RecList
  {
    virtual ~RecListT() {}

    typedef std::map<std::string,X> MapType;

    MapType list;

    // Load one object of this type
    void load(ESMReader &esm, const std::string &id)
    {
      std::string id2 = toLower (id);
      list[id2].load(esm);
    }

    // Find the given object ID, or return NULL if not found.
    const X* search(const std::string &id) const
    {
        std::string id2 = toLower (id);

        typename MapType::const_iterator iter = list.find (id2);

        if (iter == list.end())
            return NULL;

        return &iter->second;
    }

    // Find the given object ID (throws an exception if not found)
    const X* find(const std::string &id) const
    {
        const X *object = search (id);

        if (!object)
            throw std::runtime_error ("object " + id + " not found");

        return object;
    }

    int getSize() { return list.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (typename MapType::const_iterator iter (list.begin()); iter!=list.end(); ++iter)
            identifier.push_back (iter->first);
    }
  };

  // Same as RecListT, but does not case-smash the IDs
  // Note that lookups (search or find) are still case insensitive
  template <typename X>
  struct RecListCaseT : RecList
  {
    virtual ~RecListCaseT() {}

    typedef std::map<std::string,X> MapType;

    MapType list;

    // Load one object of this type
    void load(ESMReader &esm, const std::string &id)
    {
        //std::string id2 = toLower (id);

        list[id].load(esm);
    }

    // Find the given object ID, or return NULL if not found.
    const X* search(const std::string &id) const
    {
        std::string id2 = toLower (id);

        for (typename MapType::const_iterator iter = list.begin();
            iter != list.end(); ++iter)
        {
            if (toLower(iter->first) == id2)
                return &iter->second;
        }

        return NULL;
    }

    // non-const version
    X* search(const std::string &id)
    {
        std::string id2 = toLower (id);

        for (typename MapType::iterator iter = list.begin();
            iter != list.end(); ++iter)
        {
            if (toLower(iter->first) == id2)
                return &iter->second;
        }

        return NULL;
    }

    // Find the given object ID (throws an exception if not found)
    const X* find(const std::string &id) const
    {
        const X *object = search (id);

        if (!object)
            throw std::runtime_error ("object " + id + " not found");

        return object;
    }

    int getSize() { return list.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (typename MapType::const_iterator iter (list.begin()); iter!=list.end(); ++iter)
            identifier.push_back (iter->first);
    }
  };

    /// Modified version of RecListT for records, that need to store their own ID
  template <typename X>
  struct RecListWithIDT : RecList
  {
    virtual ~RecListWithIDT() {}

    typedef std::map<std::string,X> MapType;

    MapType list;

    // Load one object of this type
    void load(ESMReader &esm, const std::string &id)
    {
      std::string id2 = toLower (id);
      list[id2].load(esm, id2);
    }

    // Find the given object ID, or return NULL if not found.
    const X* search(const std::string &id) const
    {
        std::string id2 = toLower (id);

        typename MapType::const_iterator iter = list.find (id2);

        if (iter == list.end())
            return NULL;
        return &iter->second;
    }

    // Find the given object ID (throws an exception if not found)
    const X* find(const std::string &id) const
    {
        const X *object = search (id);

        if (!object)
            throw std::runtime_error ("object " + id + " not found");

        return object;
    }

    int getSize() { return list.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (typename MapType::const_iterator iter (list.begin()); iter!=list.end(); ++iter)
            identifier.push_back (iter->first);
    }
  };

  // The only difference to the above is a slight change to the load()
  // function. We might merge these together later, and store the id
  // in all the structs.
  template <typename X>
  struct RecIDListT : RecList
  {
    virtual ~RecIDListT() {}

    typedef std::map<std::string,X> MapType;

    MapType list;

    void load(ESMReader &esm, const std::string &id)
    {
      std::string id2 = toLower (id);
      X& ref = list[id2];

      ref.id = id;
      ref.load(esm);
    }

    // Find the given object ID, or return NULL if not found.
    const X* search(const std::string &id) const
    {
        std::string id2 = toLower (id);

        typename MapType::const_iterator iter = list.find (id2);

        if (iter == list.end())
            return NULL;

        return &iter->second;
    }

    // Find the given object ID (throws an exception if not found)
    const X* find(const std::string &id) const
    {
        const X *object = search (id);

        if (!object)
            throw std::runtime_error ("object " + id + " not found");

        return object;
    }

    int getSize() { return list.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (typename MapType::const_iterator iter (list.begin()); iter!=list.end(); ++iter)
            identifier.push_back (iter->first);
    }
  };

  /* Land textures are indexed by an integer number
   */
  struct LTexList : RecList
  {
    virtual ~LTexList() {}

    // TODO: For multiple ESM/ESP files we need one list per file.
    std::vector<LandTexture> ltex;

    LTexList()
    {
      // More than enough to hold Morrowind.esm.
      ltex.reserve(128);
    }

    const LandTexture* search(size_t index) const
    {
        assert(index < ltex.size());
        return &ltex.at(index);
    }

    int getSize() { return ltex.size(); }
    int getSize() const { return ltex.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const {}

    void load(ESMReader &esm, const std::string &id)
    {
      LandTexture lt;
      lt.load(esm);
      lt.id = id;

      // Make sure we have room for the structure
      if(lt.index + 1 > (int)ltex.size())
        ltex.resize(lt.index+1);

      // Store it
      ltex[lt.index] = lt;
    }
  };

  /* Landscapes are indexed by the X,Y coordinates of the exterior
     cell they belong to.
   */
  struct LandList : RecList
  {
    virtual ~LandList()
    {
      for ( LandMap::iterator itr = lands.begin(); itr != lands.end(); ++itr )
      {
          delete itr->second;
      }
    }

    // Map containing all landscapes
    typedef std::pair<int, int> LandCoord;
    typedef std::map<LandCoord, Land*> LandMap;
    LandMap lands;

    int count;
    LandList() : count(0) {}
    int getSize() { return count; }

    virtual void listIdentifier (std::vector<std::string>& identifier) const {}

    // Find land for the given coordinates. Return null if no data.
    Land *search(int x, int y) const
    {
      LandMap::const_iterator itr = lands.find(std::make_pair<int, int>(x, y));
      if ( itr == lands.end() )
      {
        return NULL;
      }

      return itr->second;
    }

    void load(ESMReader &esm, const std::string &id)
    {
      count++;

      // Create the structure and load it. This actually skips the
      // landscape data and remembers the file position for later.
      Land *land = new Land();
      land->load(esm);

      // Store the structure
      lands[std::make_pair<int, int>(land->X, land->Y)] = land;
    }
  };

    struct ciLessBoost : std::binary_function<std::string, std::string, bool>
{
    bool operator() (const std::string & s1, const std::string & s2) const {
                                               //case insensitive version of is_less
        return lexicographical_compare(s1, s2, is_iless());
    }
};


  // Cells aren't simply indexed by name. Exterior cells are treated
  // separately.
  // TODO: case handling (cell names are case-insensitive, but they are also showen to the
  // player, so we can't simply smash case.
  struct CellList : RecList
  {
    // Total cell count. Used for statistics.
    int count;
    CellList() : count(0) {}
    int getSize() { return count; }

    // List of interior cells. Indexed by cell name.
    typedef std::map<std::string,ESM::Cell*, ciLessBoost> IntCells;
    IntCells intCells;

    // List of exterior cells. Indexed as extCells[gridX][gridY].
    typedef std::map<std::pair<int, int>, ESM::Cell*> ExtCells;
    ExtCells extCells;

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (IntCells::const_iterator iter (intCells.begin()); iter!=intCells.end(); ++iter)
            identifier.push_back (iter->first);
    }

    virtual ~CellList()
    {
      for (IntCells::iterator it = intCells.begin(); it!=intCells.end(); ++it)
        delete it->second;

      for (ExtCells::iterator it = extCells.begin(); it!=extCells.end(); ++it)
          delete it->second;
    }

    const ESM::Cell* searchInt(const std::string &id) const
    {
      IntCells::const_iterator iter = intCells.find(id);

      if (iter!=intCells.end())
        return iter->second;

      return 0;
    }

    const ESM::Cell* findInt(const std::string &id) const
    {
      const ESM::Cell *cell = searchInt (id);

      if (!cell)
         throw std::runtime_error ("Interior cell not found - " + id);

      return cell;
    }

    const ESM::Cell *searchExt (int x, int y) const
    {
        ExtCells::const_iterator it = extCells.find (std::make_pair (x, y));

        if (it==extCells.end())
            return 0;

        return it->second;
    }

    const ESM::Cell *findExt (int x, int y) const
    {
        const ESM::Cell *cell = searchExt (x, y);

        if (!cell)
            throw std::runtime_error ("Exterior cell not found");

        return cell;
    }
    const ESM::Cell *searchExtByName (const std::string& id) const
    {
        for (ExtCells::const_iterator iter = extCells.begin(); iter!=extCells.end(); ++iter)
        {
            if (toLower (iter->second->name) == toLower (id))
                return iter->second;
        }

        return 0;
    }

    const ESM::Cell *searchExtByRegion (const std::string& id) const
    {
        std::string id2 = toLower (id);

        for (ExtCells::const_iterator iter = extCells.begin(); iter!=extCells.end(); ++iter)
            if (toLower (iter->second->region)==id)
                return iter->second;

        return 0;
    }

    void load(ESMReader &esm, const std::string &id)
    {
      count++;

      // All cells have a name record, even nameless exterior cells.
      ESM::Cell *cell = new ESM::Cell;
      cell->name = id;

      // The cell itself takes care of all the hairy details
      cell->load(esm);

      if(cell->data.flags & ESM::Cell::Interior)
        {
          // Store interior cell by name
          intCells[id] = cell;
        }
      else
        {
          // Store exterior cells by grid position
          extCells[std::make_pair (cell->data.gridX, cell->data.gridY)] = cell;
        }
    }
  };

  struct PathgridList : RecList
  {
      int count;

      // List of grids for interior cells. Indexed by cell name.
      typedef std::map<std::string,ESM::Pathgrid*, ciLessBoost> IntGrids;
      IntGrids intGrids;

      // List of grids for exterior cells. Indexed as extCells[gridX][gridY].
      typedef std::map<std::pair<int, int>, ESM::Pathgrid*> ExtGrids;
      ExtGrids extGrids;

      PathgridList() : count(0) {}

      virtual ~PathgridList()
      {
          for (IntGrids::iterator it = intGrids.begin(); it!=intGrids.end(); ++it)
              delete it->second;

          for (ExtGrids::iterator it = extGrids.begin(); it!=extGrids.end(); ++it)
              delete it->second;
      }

      int getSize() { return count; }

      virtual void listIdentifier (std::vector<std::string>& identifier) const
      {
          // do nothing
      }

      void load(ESMReader &esm, const std::string &id)
      {
          count++;
          ESM::Pathgrid *grid = new ESM::Pathgrid;
          grid->load(esm);
          if (grid->data.x == 0 && grid->data.y == 0)
          {
              intGrids[grid->cell] = grid;
          }
          else
          {
              extGrids[std::make_pair(grid->data.x, grid->data.y)] = grid;
          }
      }

      Pathgrid *find(int cellX, int cellY, const std::string &cellName) const
      {
          Pathgrid *result = search(cellX, cellY, cellName);
          if (!result)
          {
              throw std::runtime_error("no pathgrid found for cell " + cellName);
          }
          return result;
      }

      Pathgrid *search(int cellX, int cellY, const std::string &cellName) const
      {
          Pathgrid *result = NULL;
          if (cellX == 0 && cellY == 0) // possibly interior
          {
              IntGrids::const_iterator it = intGrids.find(cellName);
              if (it != intGrids.end())
                result = it->second;
          }
          else
          {
              ExtGrids::const_iterator it = extGrids.find(std::make_pair(cellX, cellY));
              if (it != extGrids.end())
                result = it->second;
          }
          return result;
      }

      Pathgrid *search(const ESM::Cell &cell) const
      {
          int cellX, cellY;
          if (cell.data.flags & ESM::Cell::Interior)
          {
              cellX = cellY = 0;
          }
          else
          {
              cellX = cell.data.gridX;
              cellY = cell.data.gridY;
          }
          return search(cellX, cellY, cell.name);
      }
  };

  template <typename X>
  struct ScriptListT : RecList
  {
    virtual ~ScriptListT() {}

    typedef std::map<std::string,X> MapType;

    MapType list;

    // Load one object of this type
    void load(ESMReader &esm, const std::string &id)
    {
      X ref;
      ref.load (esm);

      std::string realId = toLower (ref.data.name.toString());

      std::swap (list[realId], ref);
    }

    // Find the given object ID, or return NULL if not found.
    const X* search(const std::string &id) const
    {
        std::string id2 = toLower (id);

        typename MapType::const_iterator iter = list.find (id2);

        if (iter == list.end())
            return NULL;

        return &iter->second;
    }

    // Find the given object ID (throws an exception if not found)
    const X* find(const std::string &id) const
    {
        const X *object = search (id);

        if (!object)
            throw std::runtime_error ("object " + id + " not found");

        return object;
    }

    int getSize() { return list.size(); }

    virtual void listIdentifier (std::vector<std::string>& identifier) const
    {
        for (typename MapType::const_iterator iter (list.begin()); iter!=list.end(); ++iter)
            identifier.push_back (iter->first);
    }
  };

  template <typename X>
  struct IndexListT
  {
        virtual ~IndexListT() {}

        typedef std::map<int, X> MapType;

        MapType list;

        void load(ESMReader &esm)
        {
            X ref;
            ref.load (esm);
            int index = ref.index;
            list[index] = ref;
        }

        int getSize()
        {
            return list.size();
        }

        virtual void listIdentifier (std::vector<std::string>& identifier) const {}

        // Find the given object ID, or return NULL if not found.
        const X* search (int id) const
        {
            typename MapType::const_iterator iter = list.find (id);

            if (iter == list.end())
                return NULL;

            return &iter->second;
        }

        // Find the given object ID (throws an exception if not found)
        const X* find (int id) const
        {
            const X *object = search (id);

            if (!object)
            {
                std::ostringstream error;
                error << "object " << id << " not found";
                throw std::runtime_error (error.str());
            }

            return object;
        }
  };

  /* We need special lists for:

     Path grids
  */
}
#endif
