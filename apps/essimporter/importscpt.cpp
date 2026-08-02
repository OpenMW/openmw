#include "importscpt.hpp"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void SCPT::load(ESM::ESMReader& esm)
    {
        esm.getHNT("SCHD", mSCHD.mName.mData, mSCHD.mNumShorts, mSCHD.mNumLongs, mSCHD.mNumFloats,
            mSCHD.mScriptDataSize, mSCHD.mStringTableSize);

        mSCRI.load(esm);

        mRunning = esm.isNextSub("RNAM");
        if (!mRunning)
            return;

        esm.getSubHeader();
        const uint32_t subSize = esm.getSubSize();
        constexpr uint32_t vanillaSize = sizeof(ESM::FormId32);
        constexpr uint32_t mwseSize = 8; // RNAM spec is 4 bytes, but MWSE can extend it to 8 bytes

        if (subSize != vanillaSize && subSize != mwseSize)
            esm.fail("record size mismatch, requested " + std::to_string(vanillaSize) + " or "
                + std::to_string(mwseSize) + ", got " + std::to_string(subSize));

        if (subSize == mwseSize)
        {
            // MWSE can store the mod index and form ID in two separate 4-byte values
            uint32_t modIndex, formId;
            esm.getT(modIndex);
            esm.getT(formId);
            mRefNum.mIndex = formId;
            mRefNum.mContentFile = static_cast<int32_t>(modIndex);
        }
        else
        {
            // Vanilla stores the mod index and form ID in a single 4-byte value
            ESM::FormId32 refNum;
            esm.getT(refNum);
            mRefNum = ESM::RefNum::fromUint32(refNum);
        }
        mRefNum.mContentFile--;
    }

}
