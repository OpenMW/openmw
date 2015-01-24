#include "importinventory.hpp"

#include <stdexcept>

#include <components/esm/esmreader.hpp>

#include <components/esm/loadcont.hpp>

namespace ESSImport
{

    void Inventory::load(ESM::ESMReader &esm)
    {
        while (esm.isNextSub("NPCO"))
        {
            ESM::ContItem contItem;
            esm.getHT(contItem);

            InventoryItem item;
            item.mId = contItem.mItem.toString();
            item.mCount = contItem.mCount;
            item.mRelativeEquipmentSlot = -1;

            // seems that a stack of items can have a set of subrecords for each item? rings0000.ess
            // doesn't make any sense to me, if the values were different then the items shouldn't stack in the first place?
            // I guess we should double check the stacking logic in OpenMW
            for (int i=0;i<std::abs(item.mCount);++i)
            {
                if (esm.isNextSub("XIDX")) // index in the stack?
                    esm.skipHSub();

                item.mSCRI.load(esm);

                // for XSOL and XCHG seen so far, but probably others too
                item.ESM::CellRef::loadData(esm);

                int charge=-1;
                esm.getHNOT(charge, "XHLT");
                item.mChargeInt = charge;
            }

            mItems.push_back(item);
        }

        // equipped items
        while (esm.isNextSub("WIDX"))
        {
            // note: same item can be equipped 2 items (e.g. 2 rings)
            // and will be *stacked* in the NPCO list, unlike openmw!
            // this is currently not handled properly.

            esm.getSubHeader();
            int itemIndex; // index of the item in the NPCO list
            esm.getT(itemIndex);

            if (itemIndex < 0 || itemIndex >= int(mItems.size()))
                esm.fail("equipment item index out of range");

            // appears to be a relative index for only the *possible* slots this item can be equipped in,
            // i.e. 0 most of the time
            int slotIndex;
            esm.getT(slotIndex);

            mItems[itemIndex].mRelativeEquipmentSlot = slotIndex;
        }
    }

}
