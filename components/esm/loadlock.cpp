#include "loadlock.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Lockpick::sRecordId = REC_LOCK;

void Lockpick::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");

    esm.getHNT(mData, "LKDT", 16);

    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}

void Lockpick::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);

    esm.writeHNT("LKDT", mData, 16);
    esm.writeHNOString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}

    void Lockpick::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mQuality = 0;
        mData.mUses = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
