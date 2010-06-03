#ifndef _GAME_RENDER_CELL_H
#define _GAME_RENDER_CELL_H

#include "../cell_store.hpp"

namespace Render
{
  class CellRender
  {
    const ESMS::CellStore *cell;

  public:
    CellRender(const ESMS::CellStore &_cell)
      : cell(&_cell) {}
  };
}

#endif
