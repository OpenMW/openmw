#ifndef OPENMW_ESSIMPORT_CONVERTINVENTORY_H
#define OPENMW_ESSIMPORT_CONVERTINVENTORY_H

#include "importinventory.hpp"

#include <components/esm3/inventorystate.hpp>

namespace ESSImport
{

    void convertInventory (const Inventory& inventory, ESM::InventoryState& state);

}

#endif
