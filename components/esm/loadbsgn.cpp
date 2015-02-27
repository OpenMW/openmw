#include "loadbsgn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int BirthSign::sRecordId = REC_BSGN;

void BirthSign::load(ESMReader &esm)
{
    mPowers.mList.clear();
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'F','N','A','M'>::value:
                mName = esm.getHString();
                break;
            case ESM::FourCC<'T','N','A','M'>::value:
                mTexture = esm.getHString();
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
                break;
            case ESM::FourCC<'N','P','C','S'>::value:
                mPowers.add(esm);
                break;
            default:
                esm.fail("Unknown subrecord");
        }
    }
}

void BirthSign::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("TNAM", mTexture);
    esm.writeHNOCString("DESC", mDescription);

    mPowers.save(esm);
}

    void BirthSign::blank()
    {
        mName.clear();
        mDescription.clear();
        mTexture.clear();
        mPowers.mList.clear();
    }

}
