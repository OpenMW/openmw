#include "loadbody.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int BodyPart::sRecordId = REC_BODY;


void BodyPart::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mRace = esm.getHNOString("FNAM");
    esm.getHNT(mData, "BYDT", 4);
}
void BodyPart::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mRace);
    esm.writeHNT("BYDT", mData, 4);
}

    void BodyPart::blank()
    {
        mData.mPart = 0;
        mData.mVampire = 0;
        mData.mFlags = 0;
        mData.mType = 0;

        mModel.clear();
        mRace.clear();
    }
}
