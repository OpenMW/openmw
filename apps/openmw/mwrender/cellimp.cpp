#include "cellimp.hpp"

#include <cassert>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

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
void insertCellRefList (CellRenderImp& cellRender, MWWorld::Environment& environment,
    T& cellRefList, ESMS::CellStore<MWWorld::RefData> &cell)
{
    if (!cellRefList.list.empty())
    {
        const MWWorld::Class& class_ =
            MWWorld::Class::get (MWWorld::Ptr (&*cellRefList.list.begin(), &cell));

        for (typename T::List::iterator it = cellRefList.list.begin();
            it != cellRefList.list.end(); it++)
        {
            if (it->mData.getCount())
                class_.insertObj (MWWorld::Ptr (&*it, &cell), cellRender, environment);
        }
    }
}

void CellRenderImp::insertCell(ESMS::CellStore<MWWorld::RefData> &cell,
    MWWorld::Environment& environment)
{
  // Loop through all references in the cell
  insertCellRefList (*this, environment, cell.activators, cell);
  insertCellRefList (*this, environment, cell.potions, cell);
  insertCellRefList (*this, environment, cell.appas, cell);
  insertCellRefList (*this, environment, cell.armors, cell);
  insertCellRefList (*this, environment, cell.books, cell);
  insertCellRefList (*this, environment, cell.clothes, cell);
  insertCellRefList (*this, environment, cell.containers, cell);
  insertCellRefList (*this, environment, cell.creatures, cell);
  insertCellRefList (*this, environment, cell.doors, cell);
  insertCellRefList (*this, environment, cell.ingreds, cell);
  //  insertCellRefList (*this, environment, cell.creatureLists, cell);
  //  insertCellRefList (*this, environment, cell.itemLists, cell);
  insertCellRefList (*this, environment, cell.lights, cell);
  insertCellRefList (*this, environment, cell.lockpicks, cell);
  insertCellRefList (*this, environment, cell.miscItems, cell);
  insertCellRefList (*this, environment, cell.npcs, cell);
  insertCellRefList (*this, environment, cell.probes, cell);
  insertCellRefList (*this, environment, cell.repairs, cell);
  insertCellRefList (*this, environment, cell.statics, cell);
  insertCellRefList (*this, environment, cell.weapons, cell);
}
