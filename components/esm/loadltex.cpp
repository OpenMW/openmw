#include "loadltex.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"
#include "util.hpp"

namespace ESM
{
    unsigned int LandTexture::sRecordId = REC_LTEX;

    LandTexture::LandTexture()
        : mIsDeleted(false)
    {}

    void LandTexture::load(ESMReader &esm)
    {
        mIsDeleted = readDeleSubRecord(esm);
        mId = esm.getHNString("NAME");
        esm.getHNT(mIndex, "INTV");
        mTexture = esm.getHNString("DATA");
    }
    void LandTexture::save(ESMWriter &esm) const
    {
        if (mIsDeleted)
        {
            writeDeleSubRecord(esm);
        }
        esm.writeHNCString("NAME", mId);
        esm.writeHNT("INTV", mIndex);
        esm.writeHNCString("DATA", mTexture);
    }

    void LandTexture::blank()
    {
        mTexture.clear();
        mIndex = -1;
        mIsDeleted = false;
    }
}
