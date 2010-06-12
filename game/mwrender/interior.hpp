#ifndef _GAME_RENDER_INTERIOR_H
#define _GAME_RENDER_INTERIOR_H

#include "cell.hpp"
#include "esm_store/cell_store.hpp"

namespace Ogre
{
  class SceneNode;
}

namespace MWRender
{
  class MWScene;
  
  /**
     This class is responsible for inserting meshes and other
     rendering objects from the given cell into the given rendering
     scene.

     TODO FIXME: Doesn't do full cleanup yet.
   */
   
  class InteriorCellRender : private CellRender
  {
    const ESMS::CellStore &cell;
    MWScene &scene;

    /// The scene node that contains all objects belonging to this
    /// cell.
    Ogre::SceneNode *base;
    
    Ogre::SceneNode *insert;

    /// start inserting a new reference.
    virtual void insertBegin (const ESM::CellRef &ref);

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh);
    
    /// insert a light related to the most recent insertBegin call.
    virtual void insertLight(float r, float g, float b, float radius);
    
    /// finish inserting a new reference and return a handle to it.
    virtual std::string insertEnd();
                   
    template<typename T>   
    void insertObj(const T& liveRef)
    {
        assert (liveRef.base != NULL);
        const std::string &model = liveRef.base->model;
        if(!model.empty())
          insertMesh ("meshes\\" + model, liveRef.ref);
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
      
    InteriorCellRender(const ESMS::CellStore &_cell, MWScene &_scene)
    : cell(_cell), scene(_scene), base(NULL), insert(NULL) {}
      
    virtual ~InteriorCellRender() { destroy(); }
  
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

