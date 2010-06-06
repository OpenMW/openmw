#ifndef _GAME_RENDER_CELL_H
#define _GAME_RENDER_CELL_H

#include "../cell_store.hpp"
#include "mwscene.hpp"

namespace Render
{
  /**
     This class is responsible for inserting meshes and other
     rendering objects from the given cell into the given rendering
     scene.

   */
  class CellRender
  {
    const ESMS::CellStore &cell;
    MWScene &scene;

    /// The scene node that contains all objects belonging to this
    /// cell.
    Ogre::SceneNode *base;

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
