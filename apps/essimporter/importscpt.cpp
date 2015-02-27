#include "importscpt.hpp"

#include <components/esm/esmreader.hpp>



namespace ESSImport
{

    void SCPT::load(ESM::ESMReader &esm)
    {
        esm.getHNT(mSCHD, "SCHD");

        mSCRI.load(esm);

        mRefNum = -1;
        if (esm.isNextSub("RNAM"))
        {
            mRunning = true;
            esm.getHT(mRefNum);
        }
        else
            mRunning = false;
    }

}
