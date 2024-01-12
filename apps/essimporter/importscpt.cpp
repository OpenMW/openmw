#include "importscpt.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void SCPT::load(ESM::ESMReader& esm)
    {
        esm.getHNT("SCHD", mSCHD.mName.mData, mSCHD.mData.mNumShorts, mSCHD.mData.mNumLongs, mSCHD.mData.mNumFloats,
            mSCHD.mData.mScriptDataSize, mSCHD.mData.mStringTableSize);

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
