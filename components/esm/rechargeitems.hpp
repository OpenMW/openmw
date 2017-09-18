#ifndef OPENMW_COMPONENTS_ESM_RECHARGEITEMS_H
#define OPENMW_COMPONENTS_ESM_RECHARGEITEMS_H

#include <vector>
#include <string>

#include "defs.hpp"
#include "../../apps/openmw/mwworld/timestamp.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct RechargeItem {
        std::string key;
        int curCharge;
        int maxCharge;
        TimeStamp mTimeStamp;
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
