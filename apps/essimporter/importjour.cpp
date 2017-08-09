#include "importjour.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void JOUR::load(ESM::ESMReader &esm)
    {
        mText = esm.getHNString("NAME");
    }

}
