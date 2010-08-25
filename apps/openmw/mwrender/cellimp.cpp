#include "cellimp.hpp"

#include <cassert>
#include <iostream>
#include <exception>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

using namespace MWRender;

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
            if (it->mData.getCount() || it->mData.isEnabled())
            {
                MWWorld::Ptr ptr (&*it, &cell);

                try
                {
                    class_.insertObj (ptr, cellRender, environment);
                    class_.enable (ptr, environment);
                }
                catch (const std::exception& e)
                {
                    std::string error ("error during rendering: ");
                    std::cerr << error + e.what() << std::endl;
                }
            }
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
  insertCellRefList (*this, environment, cell.creatureLists, cell);
  insertCellRefList (*this, environment, cell.itemLists, cell);
  insertCellRefList (*this, environment, cell.lights, cell);
  insertCellRefList (*this, environment, cell.lockpicks, cell);
  insertCellRefList (*this, environment, cell.miscItems, cell);
  insertCellRefList (*this, environment, cell.npcs, cell);
  insertCellRefList (*this, environment, cell.probes, cell);
  insertCellRefList (*this, environment, cell.repairs, cell);
  insertCellRefList (*this, environment, cell.statics, cell);
  insertCellRefList (*this, environment, cell.weapons, cell);
}
