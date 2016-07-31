#include "importcntc.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CNTC::load(ESM::ESMReader &esm)
    {
        mIndex = 0;
        esm.getHNT(mIndex, "INDX");

        mInventory.load(esm);
    }

}
