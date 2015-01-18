#include "importcellref.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void CellRef::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mRefNum.mIndex, "FRMR"); // TODO: adjust RefNum

        // this is required since openmw supports more than 255 content files
        int pluginIndex = (mRefNum.mIndex & 0xff000000) >> 24;
        mRefNum.mContentFile = pluginIndex-1;
        mRefNum.mIndex &= 0x00ffffff;

        mIndexedRefId = esm.getHNString("NAME");

        esm.getHNT(mACDT, "ACDT");

        ACSC acsc;
        esm.getHNOT(acsc, "ACSC");
        esm.getHNOT(acsc, "ACSL");

        if (esm.isNextSub("CRED"))
            esm.skipHSub();

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();

        mEnabled = true;
        esm.getHNOT(mEnabled, "ZNAM");

        esm.getHNOT(mPos, "DATA", 24);
    }

}
