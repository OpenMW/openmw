#ifndef _GAME_RENDER_CELLIMP_H
#define _GAME_RENDER_CELLIMP_H

#include <string>

namespace ESM
{
  class CellRef;
}

namespace ESMS
{
  class CellStore;
}

namespace MWRender
{
  /// Base class for cell render, that implements inserting references into a cell in a
  /// cell type- and render-engine-independent way.

  class CellRenderImp
  {
  public:
    CellRenderImp() {}
    virtual ~CellRenderImp() {}

    /// start inserting a new reference.
    virtual void insertBegin (const ESM::CellRef &ref) = 0;

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh) = 0;
    
    /// insert a light related to the most recent insertBegin call.
    virtual void insertLight(float r, float g, float b, float radius) = 0;
    
    /// finish inserting a new reference and return a handle to it.
    virtual std::string insertEnd() = 0;
      
    void insertCell(const ESMS::CellStore &cell);

  };
}

#endif
