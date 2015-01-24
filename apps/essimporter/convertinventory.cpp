#include "convertinventory.hpp"

#include <components/misc/stringops.hpp>

namespace ESSImport
{

    void convertInventory(const Inventory &inventory, ESM::InventoryState &state)
    {
        int index = 0;
        for (std::vector<Inventory::InventoryItem>::const_iterator it = inventory.mItems.begin();
             it != inventory.mItems.end(); ++it)
        {
            ESM::ObjectState objstate;
            objstate.blank();
            objstate.mRef = *it;
            objstate.mRef.mRefID = Misc::StringUtils::lowerCase(it->mId);
            objstate.mCount = std::abs(it->mCount); // restocking items have negative count in the savefile
                                                    // openmw handles them differently, so no need to set any flags
            state.mItems.push_back(objstate);
            if (it->mRelativeEquipmentSlot != -1)
                // Note we should really write the absolute slot here, which we do not know about
                // Not a big deal, OpenMW will auto-correct to a valid slot, the only problem is when
                // an item could be equipped in two different slots (e.g. equipped two rings)
                state.mEquipmentSlots[index] = it->mRelativeEquipmentSlot;
            ++index;
        }
    }

}
