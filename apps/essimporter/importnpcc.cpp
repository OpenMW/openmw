#include "importnpcc.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void NPCC::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mNPDT, "NPDT");

        if (esm.isNextSub("AI_E"))
            esm.skipHSub();
        if (esm.isNextSub("AI_T"))
            esm.skipHSub();
        if (esm.isNextSub("AI_F"))
            esm.skipHSub();

        mInventory.load(esm);
    }

}
