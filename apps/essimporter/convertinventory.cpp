#include "convertinventory.hpp"

#include <components/misc/stringops.hpp>
#include <cstdlib>

namespace ESSImport
{

    void convertInventory(const Inventory &inventory, ESM::InventoryState &state)
    {
        int index = 0;
        for (const auto & item : inventory.mItems)
        {
            ESM::ObjectState objstate;
            objstate.blank();
            objstate.mRef = item;
            objstate.mRef.mRefID = Misc::StringUtils::lowerCase(item.mId);
            objstate.mCount = std::abs(item.mCount); // restocking items have negative count in the savefile
                                                    // openmw handles them differently, so no need to set any flags
            state.mItems.push_back(objstate);
            if (item.mRelativeEquipmentSlot != -1)
                // Note we should really write the absolute slot here, which we do not know about
                // Not a big deal, OpenMW will auto-correct to a valid slot, the only problem is when
                // an item could be equipped in two different slots (e.g. equipped two rings)
                state.mEquipmentSlots[index] = item.mRelativeEquipmentSlot;
            ++index;
        }
    }

}
