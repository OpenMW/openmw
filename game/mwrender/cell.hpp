#ifndef _GAME_RENDER_CELL_H
#define _GAME_RENDER_CELL_H

#include <cassert>

#include "esm_store/cell_store.hpp"
#include "mwscene.hpp"

namespace MWRender
{
  /**
     This class is responsible for inserting meshes and other
     rendering objects from the given cell into the given rendering
     scene.

     TODO FIXME: Doesn't do full cleanup yet.
   */
  class CellRender
  {
    const ESMS::CellStore &cell;
    MWScene &scene;

    /// The scene node that contains all objects belonging to this
    /// cell.
    Ogre::SceneNode *base;

    void insertMesh(const std::string mesh,    // NIF file
                    const ESMS::CellRef &ref); // Reference information
                   
    template<typename T>   
    void insertObj(const T& liveRef)
    {
        assert (liveRef.base != NULL);
        insertMesh ("meshes\\" + liveRef.base->model, liveRef.ref);
    }
    
    template<typename T>
    void insertCellRefList (const T& cellRefList)
    {
      for(typename T::List::const_iterator it = cellRefList.list.begin();
          it != cellRefList.list.end(); it++)
      {
        insertObj (*it);
      }    
    }
                      
  public:
    CellRender(const ESMS::CellStore &_cell,
               MWScene &_scene)
      : cell(_cell), scene(_scene), base(NULL) {}
    ~CellRender() { destroy(); }

    /// Make the cell visible. Load the cell if necessary.
    void show();

    /// Remove the cell from rendering, but don't remove it from
    /// memory.
    void hide();

    /// Destroy all rendering objects connected with this cell.
    void destroy();
  };
}

#endif
