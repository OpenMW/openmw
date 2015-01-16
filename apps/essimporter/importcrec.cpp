#include "importcrec.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CREC::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mIndex, "INDX");
    }

}
