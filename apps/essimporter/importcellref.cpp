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

        // the following two occur in ESM::CellRef too (Charge and Gold),
        // but may have entirely different meanings here
        int intv;
        esm.getHNOT(intv, "INTV");
        int nam9;
        esm.getHNOT(nam9, "NAM9");

        mActorData.load(esm);

        mEnabled = true;
        esm.getHNOT(mEnabled, "ZNAM");

        esm.getHNT(mPos, "DATA", 24);
    }

}
