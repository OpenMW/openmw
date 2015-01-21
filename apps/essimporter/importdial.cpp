#include "importdial.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void DIAL::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mIndex, "XIDX");
    }

}
