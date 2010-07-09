#include "cellimp.hpp"

#include <cassert>

using namespace MWRender;

template<typename T>
void insertObj(CellRenderImp& cellRender, T& liveRef)
{
  assert (liveRef.base != NULL);
  const std::string &model = liveRef.base->model;
  if(!model.empty())
  {
    cellRender.insertBegin (liveRef.ref);
    cellRender.insertMesh ("meshes\\" + model);
    liveRef.mData.setHandle (cellRender.insertEnd (liveRef.mData.isEnabled()));
  }
}

template<>
void insertObj(CellRenderImp& cellRender, ESMS::LiveCellRef<ESM::Light, MWWorld::RefData>& liveRef)
{
  assert (liveRef.base != NULL);
  const std::string &model = liveRef.base->model;
  if(!model.empty())
  {
    cellRender.insertBegin (liveRef.ref);

    cellRender.insertMesh ("meshes\\" + model);

    // Extract the color and convert to floating point
    const int color = liveRef.base->data.color;
    const float r = ((color >>  0) & 0xFF) / 255.0f;
    const float g = ((color >>  8) & 0xFF) / 255.0f;
    const float b = ((color >> 16) & 0xFF) / 255.0f;
    const float radius = float(liveRef.base->data.radius);
    cellRender.insertLight(r, g, b, radius);

    liveRef.mData.setHandle (cellRender.insertEnd (liveRef.mData.isEnabled()));
  }
}

template<typename T>
void insertCellRefList (CellRenderImp& cellRender, T& cellRefList)
{
  for(typename T::List::iterator it = cellRefList.list.begin();
    it != cellRefList.list.end(); it++)
  {
    insertObj (cellRender, *it);
  }
}

void CellRenderImp::insertCell(ESMS::CellStore<MWWorld::RefData> &cell)
{
  // Loop through all references in the cell
  insertCellRefList (*this, cell.activators);
  insertCellRefList (*this, cell.potions);
  insertCellRefList (*this, cell.appas);
  insertCellRefList (*this, cell.armors);
  insertCellRefList (*this, cell.books);
  insertCellRefList (*this, cell.clothes);
  insertCellRefList (*this, cell.containers);
  insertCellRefList (*this, cell.creatures);
  insertCellRefList (*this, cell.doors);
  insertCellRefList (*this, cell.ingreds);
//  insertCellRefList (*this, cell.creatureLists);
//  insertCellRefList (*this, cell.itemLists);
  insertCellRefList (*this, cell.lights);
  insertCellRefList (*this, cell.lockpicks);
  insertCellRefList (*this, cell.miscItems);
  insertCellRefList (*this, cell.npcs);
  insertCellRefList (*this, cell.probes);
  insertCellRefList (*this, cell.repairs);
  insertCellRefList (*this, cell.statics);
  insertCellRefList (*this, cell.weapons);
}


