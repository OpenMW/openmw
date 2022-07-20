#include "importinfo.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void INFO::load(ESM::ESMReader &esm)
    {
        mInfo = esm.getHNString("INAM");
        mActorRefId = esm.getHNString("ACDT");
    }

}
