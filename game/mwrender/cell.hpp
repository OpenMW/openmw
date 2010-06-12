#ifndef _GAME_RENDER_CELL_H
#define _GAME_RENDER_CELL_H

#include <cassert>

#include "esm_store/cell_store.hpp"
#include "mwscene.hpp"

namespace MWRender
{
  /// Base class for cell render, that implements inserting references into a cell in a
  /// cell type- and render-engine-independent way.

  class CellRender
  {
  public:
    CellRender() {}
    virtual ~CellRender() {}

    /// start inserting a new reference.
    virtual void insertBegin (const ESMS::CellRef &ref) = 0;

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh) = 0;
    
    /// finish inserting a new reference and return a handle to it.
    virtual std::string insertEnd() = 0;
      
    void insertCell(const ESMS::CellStore &cell);
  };
  
  template<typename T>   
  void insertObj(CellRender& cellRender, const T& liveRef)
  {
      assert (liveRef.base != NULL);
      const std::string &model = liveRef.base->model;
      if(!model.empty())
      {
        cellRender.insertBegin (liveRef.ref);
        cellRender.insertMesh ("meshes\\" + model);
        cellRender.insertEnd();
      }
  }

  template<>   
  void insertObj(CellRender& cellRender, const ESMS::LiveCellRef<ESM::Light>& liveRef);
    
  template<typename T>
  void insertCellRefList (CellRender& cellRender, const T& cellRefList)
  {
    for(typename T::List::const_iterator it = cellRefList.list.begin();
        it != cellRefList.list.end(); it++)
    {
      insertObj (cellRender, *it);
    }    
  }    
}

#endif
