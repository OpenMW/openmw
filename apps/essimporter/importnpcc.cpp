#include "importnpcc.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void NPCC::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mNPDT, "NPDT");

        // container:
        // XIDX
        // XHLT - condition
        // WIDX - equipping?
    }

}
