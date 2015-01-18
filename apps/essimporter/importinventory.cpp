#include "importinventory.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void Inventory::load(ESM::ESMReader &esm)
    {
        while (esm.isNextSub("NPCO"))
        {
            InventoryItem item;
            item.mId = esm.getHString();

            if (esm.isNextSub("XIDX"))
                esm.skipHSub();

            std::string script = esm.getHNOString("SCRI");
            // script variables?
            // unsure if before or after ESM::CellRef
            if (!script.empty())
            {
                if (esm.isNextSub("SLCS"))
                    esm.skipHSub();
                if (esm.isNextSub("SLSD")) // Short Data?
                    esm.skipHSub();
                if (esm.isNextSub("SLFD")) // Float Data?
                    esm.skipHSub();
            }

            // for XSOL and XCHG seen so far, but probably others too
            item.ESM::CellRef::loadData(esm);

            item.mCondition = -1;
            esm.getHNOT(item.mCondition, "XHLT");
            mItems.push_back(item);
        }

        while (esm.isNextSub("WIDX"))
        {
            // equipping?
            esm.skipHSub();
        }
    }

}
