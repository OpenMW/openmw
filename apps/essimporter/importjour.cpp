#include "importjour.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void JOUR::load(ESM::ESMReader &esm)
    {
        mText = esm.getHNString("NAME");
    }

}
