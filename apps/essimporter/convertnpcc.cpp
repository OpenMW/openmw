#include "convertnpcc.hpp"

#include "convertinventory.hpp"

namespace ESSImport
{

    void convertNPCC(const NPCC &npcc, ESM::NpcState &npcState)
    {
        npcState.mNpcStats.mReputation = npcc.mNPDT.mReputation;

        convertInventory(npcc.mInventory, npcState.mInventory);
    }
}
