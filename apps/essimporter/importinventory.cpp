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
            ContItem contItem;
            esm.getHT(contItem);

            InventoryItem item;
            item.mId = contItem.mItem.toString();
            item.mCount = contItem.mCount;
            item.mRelativeEquipmentSlot = -1;
            item.mLockLevel = 0;

            unsigned int itemCount = std::abs(item.mCount);
            bool separateStacks = false;
            for (unsigned int i=0;i<itemCount;++i)
            {
                bool newStack = esm.isNextSub("XIDX");
                if (newStack)
                {
                    unsigned int idx;
                    esm.getHT(idx);
                    separateStacks = true;
                    item.mCount = 1;
                }

                item.mSCRI.load(esm);

                // for XSOL and XCHG seen so far, but probably others too
                bool isDeleted = false;
                item.ESM::CellRef::loadData(esm, isDeleted);

                int charge=-1;
                esm.getHNOT(charge, "XHLT");
                item.mChargeInt = charge;

                if (newStack)
                    mItems.push_back(item);
            }

            if (!separateStacks)
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
