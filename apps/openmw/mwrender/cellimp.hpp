#ifndef _GAME_RENDER_CELLIMP_H
#define _GAME_RENDER_CELLIMP_H

#include <string>

#include "components/esm_store/cell_store.hpp"

#include "../mwworld/refdata.hpp"

namespace ESM
{
  class CellRef;
}

namespace MWWorld
{
    class Environment;
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
    virtual void insertBegin (ESM::CellRef &ref) = 0;

    /// insert a mesh related to the most recent insertBegin call.
    virtual void insertMesh(const std::string &mesh) = 0;

    /// insert a light related to the most recent insertBegin call.
    virtual void insertLight(float r, float g, float b, float radius) = 0;

    /// finish inserting a new reference and return a handle to it.
    virtual std::string insertEnd (bool Enable) = 0;

    void insertCell(ESMS::CellStore<MWWorld::RefData> &cell, MWWorld::Environment& environment);

  };

    /// Exception-safe rendering
    class Rendering
    {
            CellRenderImp& mCellRender;
            bool mEnd;

            // not implemented
            Rendering (const Rendering&);
            Rendering& operator= (const Rendering&);

        public:

            Rendering (CellRenderImp& cellRender, ESM::CellRef &ref)
            : mCellRender (cellRender), mEnd (false)
            {
                mCellRender.insertBegin (ref);
            }

            ~Rendering()
            {
                if (!mEnd)
                    mCellRender.insertEnd (false);
            }

            std::string end (bool enable)
            {
                assert (!mEnd);
                mEnd = true;
                return mCellRender.insertEnd (enable);
            }
    };
}

#endif
