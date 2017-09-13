#ifndef OPENMW_COMPONENTS_ESM_RECHARGEITEMS_H
#define OPENMW_COMPONENTS_ESM_RECHARGEITEMS_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RechargeItem {
        std::string key;
        int curCharge;
        int maxCharge;
    };

    // format 0, saved games only
    struct RechargeItems
    {
        std::vector<RechargeItem> mRechargeItems;

        void load(ESM::ESMReader& esm);
        void write(ESM::ESMWriter& esm) const;
    };

}

#endif
