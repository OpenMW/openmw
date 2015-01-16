#include "importcellref.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CellRef::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mRefNum.mIndex, "FRMR"); // TODO: adjust RefNum

        mIndexedRefId = esm.getHNString("NAME");

        esm.getHNT(mACDT, "ACDT");

        ACSC acsc;
        esm.getHNOT(acsc, "ACSC");
        esm.getHNOT(acsc, "ACSL");

        if (esm.isNextSub("CRED"))
            esm.skipHSub();

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();
        esm.getHNOT(mPos, "DATA", 24);
    }

}
