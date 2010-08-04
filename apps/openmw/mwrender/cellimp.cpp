#include "cellimp.hpp"

#include <cassert>

using namespace MWRender;

template<typename T>
void insertObj(CellRenderImp& cellRender, T& liveRef, const ESMS::ESMStore& store)
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
void insertObj(CellRenderImp& cellRender, ESMS::LiveCellRef<ESM::Light, MWWorld::RefData>& liveRef, const ESMS::ESMStore& store)
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

template<>
void insertObj(CellRenderImp& cellRender, ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>& liveRef, const ESMS::ESMStore& store)
{
    std::string headID = liveRef.base->head;

    //get the part of the bodypart id which describes the race and the gender
    std::string bodyRaceID = headID.substr(0, headID.find_last_of("head_") - 4);
    std::string headModel = "meshes\\" + store.bodyParts.find(headID)->model;

    cellRender.insertBegin(liveRef.ref);
    cellRender.insertMesh(headModel);

    //TODO: define consts for each bodypart e.g. chest, foot, wrist... and put the parts in the right place
    cellRender.insertMesh("meshes\\" + store.bodyParts.find(bodyRaceID + "chest")->model);

    liveRef.mData.setHandle (cellRender.insertEnd (liveRef.mData.isEnabled()));
}

template<typename T>
void insertCellRefList (CellRenderImp& cellRender, const ESMS::ESMStore& store, T& cellRefList)
{
    for(typename T::List::iterator it = cellRefList.list.begin();
        it != cellRefList.list.end(); it++)
    {
        if (it->mData.getCount())
            insertObj (cellRender, *it, store);
    }
}

void CellRenderImp::insertCell(ESMS::CellStore<MWWorld::RefData> &cell, const ESMS::ESMStore& store)
{
  // Loop through all references in the cell
  insertCellRefList (*this, store, cell.activators);
  insertCellRefList (*this, store, cell.potions);
  insertCellRefList (*this, store, cell.appas);
  insertCellRefList (*this, store, cell.armors);
  insertCellRefList (*this, store, cell.books);
  insertCellRefList (*this, store, cell.clothes);
  insertCellRefList (*this, store, cell.containers);
  insertCellRefList (*this, store, cell.creatures);
  insertCellRefList (*this, store, cell.doors);
  insertCellRefList (*this, store, cell.ingreds);
  //  insertCellRefList (*this, store, cell.creatureLists);
  //  insertCellRefList (*this, store, cell.itemLists);
  insertCellRefList (*this, store, cell.lights);
  insertCellRefList (*this, store, cell.lockpicks);
  insertCellRefList (*this, store, cell.miscItems);
  insertCellRefList (*this, store, cell.npcs);
  insertCellRefList (*this, store, cell.probes);
  insertCellRefList (*this, store, cell.repairs);
  insertCellRefList (*this, store, cell.statics);
  insertCellRefList (*this, store, cell.weapons);
}
