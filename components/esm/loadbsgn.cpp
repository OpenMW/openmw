#include "loadbsgn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int BirthSign::sRecordId = REC_BSGN;

void BirthSign::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");
    mTexture = esm.getHNOString("TNAM");
    mDescription = esm.getHNOString("DESC");

    mPowers.load(esm);
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
