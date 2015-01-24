#include "importscpt.hpp"

#include <components/esm/esmreader.hpp>



namespace ESSImport
{

    void SCPT::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mSCHD, "SCHD");

        mSCRI.load(esm);

        mRNAM = -1;
        if (esm.isNextSub("RNAM"))
        {
            mHasRNAM = true;
            esm.getHT(mRNAM);
        }
        else
            mHasRNAM = false;
    }

}
