#include "importscpt.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void SCPT::load(ESM::ESMReader& esm)
    {
        esm.getHNT("SCHD", mSCHD.mName.mData, mSCHD.mNumShorts, mSCHD.mNumLongs, mSCHD.mNumFloats,
            mSCHD.mScriptDataSize, mSCHD.mStringTableSize);

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
