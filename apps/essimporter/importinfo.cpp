#include "importinfo.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void INFO::load(ESM::ESMReader& esm)
    {
        if (esm.peekNextSub("XNAM"))
        {
            // TODO: Support older saves by turning XNAM into a RefId.
            // XNAM is probably the number of the topic response within the topic record's linked list.
            // Resolving this value will likely require loading Morrowind.esm.

            esm.getSubName();
            esm.skipHSub();
            mInfo = ESM::RefId();
        }
        else
            mInfo = esm.getHNRefId("INAM");

        mActorRefId = esm.getHNString("ACDT");
    }

}
