#include "importinfo.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void INFO::load(ESM::ESMReader &esm)
    {
        mInfo = esm.getHNString("INAM");
        mActorRefId = esm.getHNString("ACDT");
    }

}
