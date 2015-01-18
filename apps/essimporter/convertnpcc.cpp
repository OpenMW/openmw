#include "convertnpcc.hpp"

namespace ESSImport
{

    void convertNPCC(const NPCC &npcc, ESM::NpcState &npcState)
    {
        npcState.mNpcStats.mReputation = npcc.mNPDT.mReputation;

        for (std::vector<NPCC::InventoryItem>::const_iterator it = npcc.mInventory.begin();
             it != npcc.mInventory.end(); ++it)
        {
            ESM::ObjectState obj;
            obj.blank();
            obj.mRef.mRefID = it->mId;
            obj.mRef.mCharge = it->mCondition;

            // Don't know type of object :( change save format?
            // npcState.mInventory.mItems.push_back(std::make_pair(obj, std::make_pair(0,0)));
        }
    }
}
