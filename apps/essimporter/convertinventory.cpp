#include "convertinventory.hpp"

#include <components/misc/stringops.hpp>

namespace ESSImport
{

    void convertInventory(const Inventory &inventory, ESM::InventoryState &state)
    {
        for (std::vector<Inventory::InventoryItem>::const_iterator it = inventory.mItems.begin();
             it != inventory.mItems.end(); ++it)
        {
            ESM::ObjectState objstate;
            objstate.blank();
            objstate.mRef.mRefID = Misc::StringUtils::lowerCase(it->mId);
            objstate.mCount = std::abs(it->mCount); // restocking items have negative count in the savefile
                                                    // openmw handles them differently, so no need to set any flags
            objstate.mRef.mCharge = it->mCondition;
            state.mItems.push_back(std::make_pair(objstate, it->mRelativeEquipmentSlot));
        }
    }

}
