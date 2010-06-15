#include "cell.hpp"

#include <cassert>

#include "esm_store/cell_store.hpp"

using namespace MWRender;

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
void insertObj(CellRender& cellRender, const ESMS::LiveCellRef<ESM::Light>& liveRef)
{
  assert (liveRef.base != NULL);
  const std::string &model = liveRef.base->model;
  if(!model.empty())
  {
    cellRender.insertBegin (liveRef.ref);
    
    cellRender.insertMesh ("meshes\\" + model);
    
    int color = liveRef.base->data.color;
    
    cellRender.insertLight(color & 255,
                           (color >> 8) & 255,
                           (color >> 16) & 255,
                           liveRef.base->data.radius);
    
    cellRender.insertEnd();
  }  
}

template<typename T>
void insertCellRefList (CellRender& cellRender, const T& cellRefList)
{
  for(typename T::List::const_iterator it = cellRefList.list.begin();
    it != cellRefList.list.end(); it++)
  {
    insertObj (cellRender, *it);
  }    
}    

void CellRender::insertCell(const ESMS::CellStore &cell)
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


