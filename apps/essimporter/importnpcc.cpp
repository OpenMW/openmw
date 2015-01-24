#include "importnpcc.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void NPCC::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mNPDT, "NPDT");

        // FIXME: use AiPackageList, need to fix getSubName()
        while (esm.isNextSub("AI_W") || esm.isNextSub("AI_E") || esm.isNextSub("AI_T") || esm.isNextSub("AI_F")
               || esm.isNextSub("AI_A"))
            esm.skipHSub();

        mInventory.load(esm);
    }

}
