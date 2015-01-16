#include "importplayer.hpp"

#include <components/esm/esmreader.hpp>

namespace ESSImport
{

    void REFR::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mRefNum.mIndex, "FRMR");

        mRefID = esm.getHNString("NAME");

        if (esm.isNextSub("STPR"))
            esm.skipHSub(); // ESS TODO

        esm.getHNT(mACDT, "ACDT");

        ACSC acsc;
        esm.getHNOT(acsc, "ACSC");
        esm.getHNOT(acsc, "ACSL");

        esm.getHNExact(mSkills, 27*2*sizeof(int), "CHRD");

        if (esm.isNextSub("ND3D"))
            esm.skipHSub(); // ESS TODO (1 byte)

        esm.getHNOT(mPos, "DATA", 24);
    }

    void PCDT::load(ESM::ESMReader &esm)
    {
        if (esm.isNextSub("PNAM"))
            esm.skipHSub();
        if (esm.isNextSub("SNAM"))
            esm.skipHSub();
        if (esm.isNextSub("NAM9"))
            esm.skipHSub();

        mBounty = 0;
        esm.getHNOT(mBounty, "CNAM");

        mBirthsign = esm.getHNOString("BNAM");

        if (esm.isNextSub("ENAM"))
            esm.skipHSub();

        while (esm.isNextSub("FNAM"))
        {
            FNAM fnam;
            esm.getHT(fnam);
            mFactions.push_back(fnam);
        }

        if (esm.isNextSub("KNAM"))
            esm.skipHSub();
    }

}
