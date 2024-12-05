#include "importnpcc.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void NPCC::load(ESM::ESMReader& esm)
    {
        esm.getHNT("NPDT", mNPDT.mDisposition, mNPDT.unknown, mNPDT.mReputation, mNPDT.unknown2, mNPDT.mIndex);

        while (esm.isNextSub("AI_W") || esm.isNextSub("AI_E") || esm.isNextSub("AI_T") || esm.isNextSub("AI_F")
            || esm.isNextSub("AI_A"))
            mAiPackages.add(esm);

        mInventory.load(esm);
    }

}
