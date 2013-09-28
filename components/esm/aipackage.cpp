#include "aipackage.hpp"

namespace ESM
{
    void AIPackageList::load(ESMReader &esm)
    {
        while (esm.hasMoreSubs()) {
            // initialize every iteration
            AIPackage pack;
                esm.getSubName();
            if (esm.retSubName() == 0x54444e43) { // CNDT
                mList.back().mCellName = esm.getHString();
            } else if (esm.retSubName() == AI_Wander) {
                pack.mType = AI_Wander;
                esm.getHExact(&pack.mWander, 14);
                mList.push_back(pack);
            } else if (esm.retSubName() == AI_Travel) {
                pack.mType = AI_Travel;
                esm.getHExact(&pack.mTravel, 16);
                mList.push_back(pack);
            } else if (esm.retSubName() == AI_Escort ||
                       esm.retSubName() == AI_Follow)
            {
                pack.mType =
                    (esm.retSubName() == AI_Escort) ? AI_Escort : AI_Follow;
                    esm.getHExact(&pack.mTarget, 48);
                mList.push_back(pack);
            } else if (esm.retSubName() == AI_Activate) {
                pack.mType = AI_Activate;
                esm.getHExact(&pack.mActivate, 33);
                mList.push_back(pack);
            } else { // not AI package related data, so leave
                 return;
            }
        }
    }
}
