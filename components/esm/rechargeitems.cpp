#include "rechargeitems.hpp"

#include "defs.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

namespace ESM
{


    void RechargeItems::write(ESMWriter &esm) const
    {
        for (std::vector<RechargeItem>::const_iterator it = mRechargeItems.begin(); it != mRechargeItems.end(); ++it)
        {
            RechargeItem item = *it;
            esm.writeHNString("KEYR", item.key);
            esm.writeHNT("CCUR", item.curCharge);
            esm.writeHNT("CMAX", item.maxCharge);
            esm.writeHNT("TIME", item.mTimeStamp);
        }
    }

    void RechargeItems::load(ESMReader &esm)
    {
        std::vector<RechargeItem> tRechageItems;
        while (esm.isNextSub("ITEM"))
        {
            RechargeItem rItem;
            rItem.key = esm.getHNString("KEYR");
            esm.getHNT(rItem.curCharge, "CCUR");
            esm.getHNT(rItem.maxCharge, "CMAX");
            esm.getHNT(rItem.mTimeStamp, "TIME");
            tRechageItems.push_back(rItem);
        }
    }

}
