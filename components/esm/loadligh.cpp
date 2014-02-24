#include "loadligh.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Light::sRecordId = REC_LIGH;

void Light::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    mIcon = esm.getHNOString("ITEX");
    assert(sizeof(mData) == 24);
    esm.getHNT(mData, "LHDT", 24);
    mScript = esm.getHNOString("SCRI");
    mSound = esm.getHNOString("SNAM");
}
void Light::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNT("LHDT", mData, 24);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("SNAM", mSound);
}

    void Light::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mTime = 0;
        mData.mRadius = 0;
        mData.mColor = 0;
        mData.mFlags = 0;
        mSound.clear();
        mScript.clear();
        mModel.clear();
        mIcon.clear();
        mName.clear();
    }
}
