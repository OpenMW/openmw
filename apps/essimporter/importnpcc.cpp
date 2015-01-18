#include "importnpcc.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void NPCC::load(ESM::ESMReader &esm)
    {
        mIndex = 0;
        esm.getHNOT(mIndex, "INDX");

        esm.getHNT(mNPDT, "NPDT");

        while (esm.isNextSub("NPCO"))
        {
            InventoryItem item;
            item.mId = esm.getHString();

            if (esm.isNextSub("XIDX"))
                esm.skipHSub();

            item.mCondition = -1;
            esm.getHNOT(item.mCondition, "XHLT");
            mInventory.push_back(item);
        }

        while (esm.isNextSub("WIDX"))
        {
            // equipping?
            esm.skipHSub();
        }
    }

}
