#include "importcrec.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CREC::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mIndex, "INDX");

        // equivalent of ESM::Creature XSCL? probably don't have to convert this,
        // since the value can't be changed
        float scale;
        esm.getHNOT(scale, "XSCL");


        while (esm.isNextSub("AI_W") || esm.isNextSub("AI_E") || esm.isNextSub("AI_T") || esm.isNextSub("AI_F")
               || esm.isNextSub("AI_A"))
            mAiPackages.add(esm);

        mInventory.load(esm);
    }

}
